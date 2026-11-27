#include "Compiler.h"

#include <iostream>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace
{
std::filesystem::path toolDirectory(const char* invocation)
{
    namespace fs = std::filesystem;
    fs::path executable = invocation;
    std::error_code error;
#ifdef _WIN32
    std::wstring buffer(32768, L'\0');
    const auto length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length && length < buffer.size()) executable = buffer.substr(0, length);
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::string buffer(size, '\0');
    if (_NSGetExecutablePath(buffer.data(), &size) == 0) executable = buffer.c_str();
#elif defined(__linux__)
    const auto path = fs::read_symlink("/proc/self/exe", error);
    if (!error) executable = path;
#endif
    return fs::canonical(executable, error).parent_path();
}
}

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
    const auto result = compiler.compileFile(argv[1], target, toolDirectory(argv[0]));

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
