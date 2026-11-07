#include "Compiler.h"

#include <fstream>
#include <memory>
#include <sstream>
#include <vector>

#include <lua.h>
#include <lualib.h>
#include <luacode.h>

namespace luasm
{
namespace
{
struct CompileContext
{
    std::ostringstream output;
    std::filesystem::path sourceDirectory;
};

CompileContext* contextFrom(lua_State* state)
{
    return static_cast<CompileContext*>(lua_touserdata(state, lua_upvalueindex(1)));
}

std::string luaString(lua_State* state, int index)
{
    if (lua_type(state, index) != LUA_TSTRING && lua_type(state, index) != LUA_TNUMBER)
        luaL_error(state, "LuASM assembly arguments must be strings or numbers");

    size_t length = 0;
    const char* value = lua_tolstring(state, index, &length);
    return std::string(value, length);
}

int emitInstruction(lua_State* state)
{
    auto* context = contextFrom(state);
    const char* mnemonic = lua_tostring(state, lua_upvalueindex(2));
    context->output << mnemonic;

    const int count = lua_gettop(state);
    if (count > 0)
        context->output << ' ';

    for (int i = 1; i <= count; ++i)
    {
        if (i > 1)
            context->output << ", ";
        context->output << luaString(state, i);
    }

    context->output << '\n';
    return 0;
}

int emitOrg(lua_State* state)
{
    auto* context = contextFrom(state);
    if (lua_gettop(state) != 1)
        return luaL_error(state, "org() expects exactly one address string");

    context->output << "org " << luaString(state, 1) << '\n';
    return 0;
}

int emitLabel(lua_State* state)
{
    auto* context = contextFrom(state);
    if (lua_gettop(state) != 1)
        return luaL_error(state, "label() expects exactly one label name");

    context->output << luaString(state, 1) << ":\n";
    return 0;
}

int emitIncbin(lua_State* state)
{
    auto* context = contextFrom(state);
    if (lua_gettop(state) != 1)
        return luaL_error(state, "incbin() expects exactly one file path");

    const std::filesystem::path relativePath(luaString(state, 1));
    const auto resolved = (context->sourceDirectory / relativePath).lexically_normal();
    if (!std::filesystem::exists(resolved))
        return luaL_error(state, "incbin file does not exist: %s", resolved.string().c_str());

    // Keep the path relative to the source file so the generated patch is portable.
    context->output << "incbin \"" << relativePath.generic_string() << "\"\n";
    return 0;
}

void registerFunction(lua_State* state, CompileContext* context, const char* name, lua_CFunction function)
{
    lua_pushlightuserdata(state, context);
    lua_pushcclosure(state, function, 1);
    lua_setglobal(state, name);
}

void registerInstruction(lua_State* state, CompileContext* context, const char* name)
{
    lua_pushlightuserdata(state, context);
    lua_pushstring(state, name);
    lua_pushcclosure(state, emitInstruction, 2);
    lua_setglobal(state, name);
}

bool hasLoromDirective(const std::string& source)
{
    std::istringstream lines(source);
    std::string line;
    while (std::getline(lines, line))
    {
        const auto first = line.find_first_not_of(" \t");
        if (first == std::string::npos)
            continue;
        return line.compare(first, 8, "--!lorom") == 0;
    }
    return false;
}

std::string readFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("Could not open input file: " + path.string());

    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}
} // namespace

CompileResult Compiler::compileFile(const std::filesystem::path& inputPath)
{
    CompileResult result;
    try
    {
        const std::string source = readFile(inputPath);
        CompileContext context;
        context.sourceDirectory = std::filesystem::absolute(inputPath).parent_path();

        if (hasLoromDirective(source))
            context.output << "lorom\n";

        lua_State* rawState = luaL_newstate();
        if (!rawState)
            throw std::runtime_error("Could not create Luau VM state");
        const std::unique_ptr<lua_State, decltype(&lua_close)> state(rawState, lua_close);
        luaL_openlibs(state.get());

        registerFunction(state.get(), &context, "org", emitOrg);
        registerFunction(state.get(), &context, "label", emitLabel);
        registerFunction(state.get(), &context, "incbin", emitIncbin);

        static const char* instructions[] = {
            "adc", "and", "asl", "bcc", "bcs", "beq", "bit", "bmi", "bne", "bpl",
            "bra", "brk", "brl", "bvc", "bvs", "clc", "cld", "cli", "clv", "cmp",
            "cop", "cpx", "cpy", "dec", "dex", "dey", "eor", "inc", "inx", "iny",
            "jmp", "jml", "jsl", "jsr", "lda", "ldx", "ldy", "lsr", "mvn", "mvp",
            "nop", "ora", "pea", "pei", "per", "pha", "phb", "phd", "phk", "php",
            "phx", "phy", "pla", "plb", "pld", "plp", "plx", "ply", "rep", "rol",
            "ror", "rti", "rtl", "rts", "sbc", "sec", "sed", "sei", "sep", "sta",
            "stp", "stx", "sty", "stz", "tax", "tay", "tcd", "tcs", "tdc", "trb",
            "tsb", "tsc", "tsx", "txa", "txs", "txy", "tya", "tyx", "wai", "wdm",
            "xba", "xce"
        };

        for (const char* instruction : instructions)
            registerInstruction(state.get(), &context, instruction);

        size_t bytecodeSize = 0;
        char* bytecode = luau_compile(source.data(), source.size(), nullptr, &bytecodeSize);
        if (!bytecode)
            throw std::runtime_error("Luau failed to compile the source file");

        const int loadStatus = luau_load(state.get(), inputPath.string().c_str(), bytecode, bytecodeSize, 0);
        free(bytecode);
        if (loadStatus != 0)
        {
            const char* message = lua_tostring(state.get(), -1);
            throw std::runtime_error(message ? message : "Could not load Luau bytecode");
        }

        const int runStatus = lua_pcall(state.get(), 0, 0, 0);
        if (runStatus != 0)
        {
            const char* message = lua_tostring(state.get(), -1);
            throw std::runtime_error(message ? message : "Error while evaluating LuASM script");
        }

        result.outputPath = inputPath;
        result.outputPath.replace_extension(".asm");
        std::ofstream outputFile(result.outputPath, std::ios::binary);
        if (!outputFile)
            throw std::runtime_error("Could not create output file: " + result.outputPath.string());

        outputFile << context.output.str();
        if (!outputFile)
            throw std::runtime_error("Failed while writing output file");

        result.success = true;
    }
    catch (const std::exception& error)
    {
        result.error = error.what();
    }

    return result;
}
} // namespace luasm
