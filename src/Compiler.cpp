#include "Compiler.h"
#include "Syntax.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <random>
#include <regex>
#include <set>
#include <sstream>

namespace luasm
{
namespace
{
namespace fs = std::filesystem;
using Names = std::map<std::string, std::string>;

struct Scope
{
    Names values, imports;
    std::set<std::string> parameters;
};

struct Unit
{
    Module module;
    Scope globals;
};

std::string lower(std::string text)
{
    for (char& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return text;
}

std::string upper(std::string text)
{
    for (char& c : text) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return text;
}

std::string commonIndent(const std::vector<std::string>& indents)
{
    if (indents.empty()) return {};
    std::string prefix = indents.front();
    for (const auto& indent : indents)
        prefix.resize(std::mismatch(prefix.begin(), prefix.end(), indent.begin(), indent.end()).first - prefix.begin());
    return prefix;
}

std::string withoutMargin(const std::string& text, const std::string& margin)
{
    return text.compare(0, margin.size(), margin) == 0 ? text.substr(margin.size()) : text;
}

fs::path libraryPath(const fs::path& from, std::string name)
{
    if (name.empty()) throw Error("empty library name");
    const bool relative = name.front() == '\\';
    if (relative) name.erase(0, 1);
    else if (name.find_first_of("/\\:") != std::string::npos)
        throw Error("explicit library paths must start with a backslash");
    std::replace(name.begin(), name.end(), '\\', '/');
    fs::path file(name);
    if (file.empty() || file.has_root_path()) throw Error("expected a relative library path");
    if (!file.has_extension()) file += ".luau";
    if (relative) return fs::canonical(from.parent_path() / file);
    for (auto dir : {from.parent_path(), fs::current_path()})
    {
        for (;; dir = dir.parent_path())
        {
            auto candidate = dir / "Libraries" / file;
            if (fs::is_regular_file(candidate)) return fs::canonical(candidate);
            if (dir == dir.parent_path()) break;
        }
    }
    throw Error("could not find Libraries/" + file.generic_string());
}

std::string expression(const std::string& source, const Scope& scope)
{
    std::string result;
    for (auto part : splitExpression(source, ".."))
    {
        if (part.compare(0, 2, "##") == 0)
        {
            auto name = trim(part.substr(2));
            if (!scope.parameters.count(name)) throw Error("## requires a function parameter, got '" + name + "'");
            result += "#" + scope.values.at(name);
        }
        else if (part.front() == '"' || part.front() == '\'')
        {
            auto value = quotedValue(part);
            if (!value.empty() && value.front() == '#') throw Error("write # operands without quotes");
            result += value;
        }
        else
        {
            if (part.find("##") != std::string::npos || part.find("#\"") != std::string::npos ||
                part.find("#'") != std::string::npos) throw Error("invalid # operand");
            // Identifiers inside addressing forms can be parameters too, e.g. (addr,S).
            // Hex digits, immediates, assembler defines and sublabels are literal tokens.
            static const std::regex token(R"([\$#!.]?[A-Za-z_][A-Za-z_0-9]*(?:\.[A-Za-z_][A-Za-z_0-9]*)*)");
            size_t last = 0;
            for (std::sregex_iterator it(part.begin(), part.end(), token), end; it != end; ++it)
            {
                size_t pos = static_cast<size_t>(it->position());
                result += part.substr(last, pos - last);
                auto found = scope.values.find(it->str());
                const bool afterDigit = pos && std::isdigit(static_cast<unsigned char>(part[pos - 1]));
                result += found == scope.values.end() || afterDigit ? it->str() : found->second;
                last = pos + it->length();
            }
            result += part.substr(last);
        }
    }
    return result;
}

std::string interpolate(const std::string& text, const Scope& scope)
{
    static const std::regex placeholder(R"(<([A-Za-z_][A-Za-z_0-9]*(?:\.[A-Za-z_][A-Za-z_0-9]*)*)>)");
    std::string out;
    size_t last = 0;
    for (std::sregex_iterator it(text.begin(), text.end(), placeholder), end; it != end; ++it)
    {
        const size_t pos = static_cast<size_t>(it->position());
        out += text.substr(last, pos - last);
        auto found = scope.values.find((*it)[1]);
        out += found == scope.values.end() ? it->str() : found->second;
        last = pos + it->length();
    }
    return out + text.substr(last);
}

class Translation
{
    Target target;
    fs::path mainFile;
    std::map<fs::path, Unit> units;
    std::vector<const Statement*> active;
    std::vector<std::string> lines;

    Unit& load(const fs::path& path)
    {
        auto [it, fresh] = units.try_emplace(path);
        Unit& unit = it->second;
        if (fresh)
        {
            unit.module.path = path;
            readModule(unit.module);
            unit.globals.imports = unit.module.imports;
            if (path != mainFile)
                for (const auto& n : unit.module.statements)
                    if (n.kind == Kind::Value)
                        unit.globals.values[n.name] = expression(n.value, unit.globals);
        }
        return unit;
    }

    void body(Unit& unit, const std::vector<Statement>& nodes, Scope& scope, const std::string& indent)
    {
        std::vector<std::string> indents;
        for (const auto& n : nodes)
            if (n.kind != Kind::Text || !n.comment.empty()) indents.push_back(n.indent);
        emit(unit, nodes, scope, indent, commonIndent(indents));
    }

    void assembly(const Statement& node, const Scope& scope, const std::string& indent)
    {
        std::vector<std::string> indents;
        for (const auto& line : node.raw)
            if (!trim(line).empty()) indents.push_back(line.substr(0, line.find_first_not_of(" \t")));
        auto margin = commonIndent(indents);
        for (const auto& line : node.raw)
            lines.push_back((line.empty() ? "" : indent) + interpolate(withoutMargin(line, margin), scope));
    }

    void instruction(const Call& call, const std::vector<std::string>& args, const std::string& indent)
    {
        static const std::set<std::string> opcodes = {
            "adc","and","asl","bcc","bcs","beq","bit","bmi","bne","bpl","bra","brk","brl",
            "bvc","bvs","clc","cld","cli","clv","cmp","cop","cpx","cpy","dec","dex","dey",
            "eor","inc","inx","iny","jml","jmp","jsl","jsr","lda","ldx","ldy","lsr","mvn",
            "mvp","nop","ora","pea","pei","per","pha","phb","phd","phk","php","phx","phy",
            "pla","plb","pld","plp","plx","ply","rep","rol","ror","rti","rtl","rts","sbc",
            "sec","sed","sei","sep","sta","stp","stx","sty","stz","tax","tay","tcd","tcs",
            "tdc","trb","tsb","tsc","tsx","txa","txs","txy","tya","tyx","wai","wdm","xba","xce"
        };
        static const std::set<std::string> directives = {
            "db","dw","dl","dd","org","base","fill","fillbyte","pad","padbyte","warnpc","incbin","incsrc"
        };
        static const std::set<std::string> asarOnly = {
            "pushpc","pullpc","freecode","freedata","autoclean","assert","sa1rom","exhirom"
        };
        static const std::set<std::string> xkasOnly = {"assume","loadpc","savepc"};
        std::string name = lower(call.name);
        auto dot = name.find('.');
        auto mnemonic = name.substr(0, dot);
        bool opcode = opcodes.count(mnemonic) != 0;
        if (opcode && dot != std::string::npos && name.substr(dot) != ".b" &&
            name.substr(dot) != ".w" && name.substr(dot) != ".l") throw Error("invalid width on " + call.name);
        if (name == "label")
        {
            if (args.size() != 1 || !identifier(args[0])) throw Error("label expects a label name");
            lines.push_back(indent + args[0] + ":");
            return;
        }
        if ((target == Target::Xkas006 && asarOnly.count(name)) ||
            (target == Target::Asar191 && xkasOnly.count(name)))
            throw Error(call.name + " is unavailable for the selected target");
        if (!opcode && !directives.count(name) && !asarOnly.count(name) && !xkasOnly.count(name) &&
            name != "lorom" && name != "hirom" && name != "header")
            throw Error("unknown template or instruction '" + call.name + "'");
        std::string method = upper(mnemonic);
        if (opcode && dot != std::string::npos) method += name.substr(dot);
        if (name == "org" || name == "lorom" || name == "hirom" || name == "sa1rom" || name == "exhirom")
            method = name;
        std::string line = indent + method;
        if (name == "org" && args.size() != 1) throw Error("org expects one address");
        if (name == "incbin" || name == "incsrc")
        {
            if (args.size() != 1 || !fs::is_regular_file(mainFile.parent_path() / args[0]))
                throw Error(name + " expects an existing file relative to the patch");
            if (target == Target::Xkas006)
            {
                if (args[0].find_first_of(" \t;") != std::string::npos)
                    throw Error("xkas 0.06 requires an unquoted include path without spaces or semicolons");
                line += " " + args[0];
            }
            else line += " \"" + args[0] + "\"";
        }
        else
        {
            for (size_t i = 0; i < args.size(); ++i)
            {
                line += i == 0 ? " " : (opcode ? "," : ", ");
                line += args[i];
            }
        }
        lines.push_back(std::move(line));
    }

    void expand(Unit& caller, const Call& call, Scope& scope, const std::string& indent)
    {
        Unit* owner = &caller;
        std::string name = call.name;
        auto dot = name.find('.');
        auto alias = scope.imports.find(name.substr(0, dot));
        bool imported = dot != std::string::npos && alias != scope.imports.end();
        if (imported)
        {
            owner = &load(libraryPath(caller.module.path, alias->second));
            name.erase(0, dot + 1);
            if (!owner->module.exports.empty()) name = owner->module.exports + "." + name;
        }
        auto fn = owner->module.functions.find(name);
        std::vector<std::string> args;
        const auto method = lower(name);
        const bool data = method == "db" || method == "dw" || method == "dl" || method == "dd";
        for (const auto& arg : call.args)
        {
            if (fn == owner->module.functions.end() && (data || method == "assert") &&
                arg.front() == '"' && splitExpression(arg, "..").size() == 1)
            {
                auto literal = quotedValue(arg);
                if (!literal.empty() && literal.front() == '#') throw Error("write # operands without quotes");
                args.push_back(arg);
            }
            else args.push_back(expression(arg, scope));
        }
        if (fn == owner->module.functions.end())
        {
            if (imported) throw Error("unknown library template " + call.name);
            instruction(call, args, indent);
            return;
        }
        const Statement& def = *fn->second;
        if (std::find(active.begin(), active.end(), &def) != active.end())
            throw Error("recursive template expansion of " + call.name);
        if (args.size() != def.call.args.size()) throw Error("wrong argument count for " + call.name);
        Scope frame = owner->globals;
        for (size_t i = 0; i < args.size(); ++i)
        {
            frame.parameters.insert(def.call.args[i]);
            frame.values[def.call.args[i]] = args[i];
        }
        active.push_back(&def);
        body(*owner, def.body, frame, indent);
        active.pop_back();
    }

    void emit(Unit& unit, const std::vector<Statement>& nodes, Scope& scope,
              const std::string& base, const std::string& margin)
    {
        for (const auto& n : nodes)
        {
            auto indent = base + withoutMargin(n.indent, margin);
            size_t first = lines.size();
            try
            {
                switch (n.kind)
                {
                case Kind::Text:
                    lines.push_back(n.indent.empty() && n.comment.empty() ? "" : indent);
                    break;
                case Kind::Function: case Kind::Table: case Kind::Export:
                    break;
                case Kind::Value: scope.values[n.name] = expression(n.value, scope); break;
                case Kind::Import: scope.imports[n.name] = n.value; break;
                case Kind::Mapping: instruction({n.value, {}}, {}, indent); break;
                case Kind::Assembly:
                    if (!n.comment.empty()) lines.push_back(indent);
                    assembly(n, scope, indent);
                    break;
                case Kind::LabelBlock:
                    lines.push_back(indent + n.name + ":");
                    body(unit, n.body, scope, indent + "\t");
                    break;
                case Kind::Call:
                    if (!n.name.empty()) lines.push_back(indent + n.name + ":");
                    expand(unit, n.call, scope, indent + (n.name.empty() ? "" : "\t"));
                    break;
                }
                if (!n.comment.empty())
                {
                    if (first == lines.size()) lines.push_back(indent);
                    if (trim(lines[first]).empty()) lines[first] += n.comment.substr(n.comment.find_first_not_of(" \t"));
                    else lines[first] += n.comment;
                }
                if (!n.closingComment.empty()) lines.push_back(indent + n.closingComment);
            }
            catch (const std::exception& e)
            {
                throw Error(unit.module.path.string() + ":" + std::to_string(n.line) + ": " + e.what());
            }
        }
    }

public:
    Translation(Target selected, fs::path path) : target(selected), mainFile(std::move(path)) {}

    std::string run()
    {
        auto& main = load(mainFile);
        emit(main, main.module.statements, main.globals, "", "");
        std::ostringstream result;
        for (size_t i = 0; i < lines.size(); ++i)
        {
            result << lines[i];
            if (i + 1 < lines.size() || main.module.finalNewline) result << main.module.newline;
        }
        return result.str();
    }
};

// A directory reservation keeps concurrent translations from sharing a temporary
// file. Renaming on the same volume publishes only a completely written patch.
void writePatch(const fs::path& output, const std::string& text)
{
    std::random_device random;
    fs::path staging;
    do
    {
        staging = output;
        staging += ".tmp-" + std::to_string(random());
    } while (!fs::create_directory(staging));
    try
    {
        const auto temp = staging / "patch";
        std::ofstream file(temp, std::ios::binary);
        file.exceptions(std::ios::failbit | std::ios::badbit);
        file << text;
        file.close();
        fs::rename(temp, output);
    }
    catch (...)
    {
        std::error_code ignored;
        fs::remove(staging / "patch", ignored);
        fs::remove(staging, ignored);
        throw;
    }
    fs::remove(staging);
}
}

CompileResult Compiler::compileFile(const fs::path& inputPath, Target target)
{
    CompileResult result;
    try
    {
        auto output = inputPath;
        output.replace_extension(".asm");
        if (fs::exists(output) && fs::equivalent(inputPath, output))
            throw Error("input and output refer to the same file");
        const auto text = Translation(target, fs::canonical(inputPath)).run();
        writePatch(output, text);
        result.outputPath = output;
        result.success = true;
    }
    catch (const std::exception& e) { result.error = e.what(); }
    return result;
}
}
