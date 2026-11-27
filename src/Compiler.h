#pragma once

#include <filesystem>
#include <string>

namespace luasm
{
enum class Target { Asar191, Xkas006 };

struct CompileResult
{
    bool success = false;
    std::filesystem::path outputPath;
    std::string error;
};

class Compiler
{
public:
    CompileResult compileFile(const std::filesystem::path& inputPath, Target target = Target::Asar191,
                              const std::filesystem::path& toolDirectory = {});
};
}
