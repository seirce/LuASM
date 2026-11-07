cmake_minimum_required(VERSION 3.20)
project(LuASM VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# Luau is embedded as libraries; users do not need a separate luau.exe.
set(LUAU_BUILD_CLI OFF CACHE BOOL "" FORCE)
set(LUAU_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LUAU_BUILD_WEB OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    Luau
    GIT_REPOSITORY https://github.com/luau-lang/luau.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(Luau)

add_executable(LuASM
    src/main.cpp
    src/Compiler.cpp
)

target_include_directories(LuASM PRIVATE src)
target_link_libraries(LuASM PRIVATE Luau.Compiler Luau.VM)

if(MSVC)
    target_compile_options(LuASM PRIVATE /W4 /permissive-)
else()
    target_compile_options(LuASM PRIVATE -Wall -Wextra -Wpedantic)
endif()
