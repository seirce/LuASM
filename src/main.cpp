#include "Compiler.h"

#include <iostream>

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: LuASM <input.luau>\n";
        return 2;
    }

    luasm::Compiler compiler;
    const auto result = compiler.compileFile(argv[1]);

    if (!result.success)
    {
        std::cerr << "LuASM error: " << result.error << '\n';
        return 1;
    }

    std::cout << "Generated " << result.outputPath.string() << '\n';
    return 0;
}
