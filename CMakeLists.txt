cmake_minimum_required(VERSION 3.16)

project(CodeGuard)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find LLVM and Clang
find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

# Call Graph and Data Flow Library
add_library(codeguard_analysis
    src/analyzer/callgraph/call_graph.cpp
    src/analyzer/dataflow/data_flow.cpp
)

# AST Analyzer Library
add_library(codeguard_ast
    src/ast/ast_analyzer.cpp
)

target_include_directories(codeguard_ast SYSTEM PRIVATE
    ${LLVM_INCLUDE_DIRS}
    ${CLANG_INCLUDE_DIRS}
)

target_link_libraries(codeguard_ast PRIVATE
    clang-cpp
    LLVM
    codeguard_analysis
)

# Main application
add_executable(codeguard
    src/main.cpp
    src/scanner/scanner.cpp
    src/analyzer/analyzer.cpp
    src/rules/strcpy_rule.cpp
    src/rules/gets_rule.cpp
    src/rules/sprintf_rule.cpp
    src/lexer/lexer.cpp
    src/cli/cli_parser.cpp
    src/reporter/terminal_reporter.cpp
    src/reporter/json_reporter.cpp
)

target_link_libraries(codeguard PRIVATE
    codeguard_ast
    codeguard_analysis
)

# Enable testing
enable_testing()

# Find GoogleTest
find_package(GTest REQUIRED)

# Automated test executable
add_executable(codeguard_tests
    tests/rule_test.cpp
    tests/cli_test.cpp
    tests/reporter_test.cpp
    tests/dataflow_test.cpp
    src/rules/strcpy_rule.cpp
    src/rules/gets_rule.cpp
    src/rules/sprintf_rule.cpp
    src/lexer/lexer.cpp
    src/cli/cli_parser.cpp
    src/reporter/terminal_reporter.cpp
    src/reporter/json_reporter.cpp
    src/analyzer/callgraph/call_graph.cpp
    src/analyzer/dataflow/data_flow.cpp
)

# Link GoogleTest
target_link_libraries(codeguard_tests
    GTest::gtest_main
)

# Discover tests automatically
include(GoogleTest)
gtest_discover_tests(codeguard_tests)

# AST Integration Test
add_executable(ast_integration_test
    src/ast/ast_integration_test.cpp
)

target_include_directories(ast_integration_test PRIVATE
    src/ast
    src/analyzer
)

target_link_libraries(ast_integration_test PRIVATE
    codeguard_ast
    codeguard_analysis
)

add_test(
    NAME ASTIntegrationTest
    COMMAND ast_integration_test
)

set_tests_properties(ASTIntegrationTest PROPERTIES
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# Prototype tool
add_executable(ast_test
    src/ast/ast_test.cpp
)

target_include_directories(ast_test SYSTEM PRIVATE
    ${LLVM_INCLUDE_DIRS}
    ${CLANG_INCLUDE_DIRS}
)

target_link_libraries(ast_test PRIVATE
    clang-cpp
    LLVM
)

# Register vulnerable examples for static analysis compilation database
add_library(vulnerable_example OBJECT EXCLUDE_FROM_ALL
    examples/vulnerable.cpp
)

add_library(ast_test_cases OBJECT EXCLUDE_FROM_ALL
    examples/ast_test_cases.cpp
)

# Installation target
install(TARGETS codeguard RUNTIME DESTINATION bin)

