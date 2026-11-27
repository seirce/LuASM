cmake_minimum_required(VERSION 3.20)
project(LuASM VERSION 1.0.1 LANGUAGES CXX)

add_library(translation STATIC src/Compiler.cpp src/Syntax.cpp)
target_compile_features(translation PUBLIC cxx_std_17)
if(MSVC)
    target_compile_options(translation PRIVATE /W4 /permissive-)
else()
    target_compile_options(translation PRIVATE -Wall -Wextra -Wpedantic)
endif()

foreach(tool IN ITEMS LuASM LuASM-asar LuASM-xkas)
    add_executable(${tool} src/main.cpp)
    target_link_libraries(${tool} PRIVATE translation)
endforeach()
target_compile_definitions(LuASM-xkas PRIVATE LUASM_XKAS)

include(CTest)
if(BUILD_TESTING)
    add_test(NAME translate COMMAND ${CMAKE_COMMAND}
        -DASAR=$<TARGET_FILE:LuASM-asar> -DXKAS=$<TARGET_FILE:LuASM-xkas>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/translate.cmake)
endif()
