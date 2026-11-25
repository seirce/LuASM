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
        std::cerr << "Usage: " << argv[0] << " <input.luau>\n";
        return 2;
    }

    luasm::Compiler compiler;
#ifdef LUASM_XKAS
    constexpr auto target = luasm::Target::Xkas006;
#else
    constexpr auto target = luasm::Target::Asar191;
#endif
    const auto result = compiler.compileFile(argv[1], target);

    if (!result.success)
    {
        std::cerr << "LuASM error: " << result.error << '\n';

#ifdef _WIN32
        // Keep failures visible when LuASM.exe is launched by double-clicking.
        DWORD consoleProcesses[2];
        if (GetConsoleProcessList(consoleProcesses, 2) == 1)
            MessageBoxA(nullptr, result.error.c_str(), "LuASM - Translation Failed",
                        MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
#endif

        return 1;
    }

    std::cout << "Generated " << result.outputPath.string() << '\n';
    return 0;
}
