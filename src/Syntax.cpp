#include "Syntax.h"

#include <algorithm>
#include <fstream>
#include <regex>

namespace luasm
{
namespace
{
const std::string id = R"([A-Za-z_][A-Za-z_0-9]*)";
const std::string qualified = id + "(?:\\." + id + ")*";
const std::regex idPattern(id);
const std::string callSignature = "(" + qualified + ")" + R"(\s*\((.*)\))";
const std::regex callPattern(callSignature);
const std::regex assignment("(" + qualified + ")" + R"(\s*=\s*(.+))");
const std::regex exported("return\\s+(" + id + ")");
const std::regex functionHeader("function\\s+" + callSignature);
const std::regex localPrefix(R"(^local\s+)");
const std::regex asmEnd(R"(end\s*--.*)");

std::string indentOf(const std::string& text)
{
    return text.substr(0, text.find_first_not_of(" \t"));
}

// Skip entire string literals in scans for comments, commas and concatenation.
size_t quoteEnd(const std::string& text, size_t start)
{
    for (size_t i = start + 1; i < text.size(); ++i)
    {
        if (text[i] == '\\') ++i;
        else if (text[i] == text[start]) return i;
    }
    throw Error("unterminated string");
}

bool isQuote(char c) { return c == '"' || c == '\''; }

void parseLine(Statement& node, std::string code)
{
    node.indent = indentOf(code);
    for (size_t i = node.indent.size(); i < code.size(); ++i)
    {
        if (isQuote(code[i])) i = quoteEnd(code, i);
        else if (code.compare(i, 2, "--") == 0)
        {
            size_t end = i + 2;
            while (end < code.size() && code[end] == '-') ++end;
            const auto gap = code.substr(0, i).find_last_not_of(" \t");
            if (gap != std::string::npos) node.comment = code.substr(gap + 1, i - gap - 1);
            node.comment += std::string(end - i - 1, ';') + code.substr(end);
            code.resize(i);
            break;
        }
    }
    node.value = std::regex_replace(trim(code), localPrefix, "");
}

void classify(Statement& node)
{
    std::smatch match;
    auto& code = node.value;
    if (code.empty()) return;
    if (std::regex_match(code, match, functionHeader))
    {
        node.kind = Kind::Function;
        node.name = match[1];
        node.call.args = splitExpression(match[2], ",");
        std::vector<std::string> seen;
        for (const auto& arg : node.call.args)
        {
            if (!identifier(arg) || std::find(seen.begin(), seen.end(), arg) != seen.end())
                throw Error("invalid or repeated parameter '" + arg + "'");
            seen.push_back(arg);
        }
        return;
    }
    if (code == "asm") { node.kind = Kind::Assembly; return; }
    if (std::regex_match(code, match, exported))
    {
        node.kind = Kind::Export;
        node.name = match[1];
        return;
    }
    if (std::regex_match(code, match, assignment))
    {
        node.name = match[1];
        std::string rhs = match[2];
        code = std::move(rhs);
        if (code == "do") { node.kind = Kind::LabelBlock; return; }
        if (code == "{}") { node.kind = Kind::Table; return; }
    }
    if (std::regex_match(code, match, callPattern))
    {
        node.call = {match[1], splitExpression(match[2], ",")};
        node.kind = Kind::Call;
        if (node.call.name == "require")
        {
            if (node.name.empty() || node.call.args.size() != 1)
                throw Error("expected an alias = require(\"Library Name\")");
            code = quotedValue(node.call.args.front());
            node.kind = Kind::Import;
        }
        return;
    }
    if (node.name.empty()) throw Error("unrecognized statement '" + code + "'");
    node.kind = Kind::Value;
}

class Parser
{
    const std::vector<std::string>& lines;
    size_t pos = 0;
public:
    explicit Parser(const std::vector<std::string>& source) : lines(source) {}

    std::vector<Statement> block(Statement* parent = nullptr)
    {
        std::vector<Statement> body;
        while (pos < lines.size())
        {
            Statement node;
            node.line = pos + 1;
            const auto& line = lines[pos++];
            const auto stripped = trim(line);
            const auto mappingEnd = stripped.find_first_of(" \t");
            const auto mapping = stripped.substr(0, mappingEnd);
            if (mapping == "--!lorom" || mapping == "--!hirom" || mapping == "--!sa1rom" ||
                mapping == "--!exhirom" || mapping == "--!header")
            {
                parseLine(node, indentOf(line) + stripped.substr(3));
                node.kind = Kind::Mapping;
                node.value = mapping.substr(3);
            }
            else
            {
                parseLine(node, line);
                if (node.value == "end")
                {
                    if (!parent) throw Error("unexpected end");
                    parent->closingComment = trimLeading(node.comment);
                    return body;
                }
                classify(node);
                if (parent && node.kind == Kind::Function)
                    throw Error("declare function templates at file scope");
            }
            if (node.kind == Kind::Function || node.kind == Kind::LabelBlock)
                node.body = block(&node);
            if (node.kind == Kind::Assembly)
            {
                while (pos < lines.size())
                {
                    const auto tail = trim(lines[pos]);
                    if (tail == "end" || std::regex_match(tail, asmEnd)) break;
                    node.raw.push_back(lines[pos++]);
                }
                if (pos == lines.size()) throw Error("missing end for asm at line " + std::to_string(node.line));
                Statement close;
                parseLine(close, lines[pos++]);
                node.closingComment = trimLeading(close.comment);
            }
            body.push_back(std::move(node));
        }
        if (parent) throw Error("missing end for block at line " + std::to_string(parent->line));
        return body;
    }

    size_t lineNumber() const { return pos; }

    static std::string trimLeading(const std::string& text)
    {
        const auto start = text.find_first_not_of(" \t");
        return start == std::string::npos ? "" : text.substr(start);
    }
};
}

std::string trim(const std::string& text)
{
    auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    return text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
}

bool identifier(const std::string& text) { return std::regex_match(text, idPattern); }

std::string quotedValue(const std::string& text)
{
    if (text.empty() || !isQuote(text.front()) || quoteEnd(text, 0) != text.size() - 1)
        throw Error("expected a quoted string");
    std::string value;
    for (size_t i = 1; i + 1 < text.size(); ++i)
    {
        if (text[i] == '\\' && (text[i+1] == '\\' || text[i+1] == text.front())) ++i;
        value += text[i];
    }
    return value;
}

std::vector<std::string> splitExpression(const std::string& text, const std::string& separator)
{
    if (trim(text).empty()) return {};
    std::vector<std::string> parts;
    std::string closers;
    size_t start = 0;
    for (size_t i = 0; i < text.size(); ++i)
    {
        const char c = text[i];
        if (isQuote(c)) i = quoteEnd(text, i);
        else if (c == '(' || c == '[') closers += c == '(' ? ')' : ']';
        else if (c == ')' || c == ']')
        {
            if (closers.empty() || closers.back() != c) throw Error("unbalanced operand");
            closers.pop_back();
        }
        else if (closers.empty() && text.compare(i, separator.size(), separator) == 0)
        {
            parts.push_back(trim(text.substr(start, i - start)));
            i += separator.size() - 1;
            start = i + 1;
        }
    }
    if (!closers.empty()) throw Error("unbalanced operand");
    parts.push_back(trim(text.substr(start)));
    if (std::find(parts.begin(), parts.end(), "") != parts.end()) throw Error("missing operand");
    return parts;
}

void readModule(Module& module)
{
    std::ifstream input(module.path, std::ios::binary);
    if (!input) throw Error("could not open " + module.path.string());
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(input, line))
    {
        module.finalNewline = !input.eof();
        if (!line.empty() && line.back() == '\r')
        {
            if (lines.empty()) module.newline = "\r\n";
            line.pop_back();
        }
        if (lines.empty() && line.compare(0, 3, "\xEF\xBB\xBF") == 0) line.erase(0, 3);
        lines.push_back(line);
    }
    if (input.bad()) throw Error("could not read " + module.path.string());
    Parser parser(lines);
    try { module.statements = parser.block(); }
    catch (const Error& e)
    {
        throw Error(module.path.string() + ":" + std::to_string(parser.lineNumber()) + ": " + e.what());
    }
    for (const auto& node : module.statements)
    {
        if (node.kind == Kind::Export) module.exports = node.name;
        if (node.kind == Kind::Import) module.imports[node.name] = node.value;
        if (node.kind == Kind::Function && !module.functions.emplace(node.name, &node).second)
            throw Error(module.path.string() + ":" + std::to_string(node.line) + ": duplicate template " + node.name);
    }
}
}
