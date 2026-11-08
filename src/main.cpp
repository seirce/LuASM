#include "Compiler.h"

#include <iostream>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

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
        const std::string errorMessage =
            "LuASM could not compile the input file.\n\n" +
            std::string(argv[1]) + "\n\n" + result.error;

        std::cerr << "LuASM error: " << result.error << '\n';

#ifdef _WIN32
        // Keep failures visible when LuASM.exe is launched by double-clicking.
        MessageBoxA(
            nullptr,
            errorMessage.c_str(),
            "LuASM - Compilation Failed",
            MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
#endif

        return 1;
    }

    std::cout << "Generated " << result.outputPath.string() << '\n';
    return 0;
}
