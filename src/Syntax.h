#pragma once

#include <filesystem>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace luasm
{
struct Error : std::runtime_error { using std::runtime_error::runtime_error; };

struct Call
{
    std::string name;
    std::vector<std::string> args;
};

struct LibraryRef
{
    std::string name;
    bool byBank = false;
};

enum class Kind { Text, Mapping, Function, LabelBlock, Assembly, Call, Import, Value, Table, Export };

struct Statement
{
    Kind kind = Kind::Text;
    size_t line = 0;
    std::string indent, comment, closingComment, name, value;
    Call call;
    LibraryRef library;
    std::vector<Statement> body;
    std::vector<std::string> raw;
};

struct Module
{
    std::filesystem::path path;
    std::vector<Statement> statements;
    std::map<std::string, const Statement*> functions;
    std::map<std::string, LibraryRef> imports;
    std::string exports, newline = "\n";
    bool finalNewline = false;
};

std::string trim(const std::string& text);
bool identifier(const std::string& text);
std::string quotedValue(const std::string& text);
std::string bankHeader(const std::string& firstLine);
std::vector<std::string> splitExpression(const std::string& text, const std::string& separator);
void readModule(Module& module);
}
