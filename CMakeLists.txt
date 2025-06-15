cmake_minimum_required(VERSION 3.10)
project(CoCompiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Useful for IDEs

# Define source files
set(SOURCE_FILES
    main.cpp
    src/Lexer.cpp
    src/Parser.cpp
    src/Compiler.cpp
    src/VM.cpp
    src/SymbolTable.cpp # Added SymbolTable source file
)

# Define include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Add the executable
add_executable(cocompiler ${SOURCE_FILES})
