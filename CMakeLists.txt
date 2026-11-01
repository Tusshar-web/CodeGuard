cmake_minimum_required(VERSION 3.16)

project(CodeGuard)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(codeguard
    src/main.cpp
    src/scanner/scanner.cpp
    src/analyzer/analyzer.cpp
    src/rules/strcpy_rule.cpp
)