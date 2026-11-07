#pragma once

#include <filesystem>
#include <string>

namespace luasm
{
struct CompileResult
{
    bool success = false;
    std::filesystem::path outputPath;
    std::string error;
};

class Compiler
{
public:
    CompileResult compileFile(const std::filesystem::path& inputPath);
};
} // namespace luasm
