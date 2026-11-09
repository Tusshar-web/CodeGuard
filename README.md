# CodeGuard 🛡️ – C++ Static Security Analyzer

**CodeGuard** is a static security analysis tool designed to identify dangerous coding practices, memory vulnerabilities, and security flaws in C and C++ source code. Powered by Clang's Abstract Syntax Tree (AST), call-graph analysis, and data-flow taint tracking, CodeGuard provides deep semantic inspection without executing code.

---

## 📑 Table of Contents

1. [Introduction](#1-introduction)
2. [Problem Statement](#2-problem-statement)
3. [Features](#3-features)
4. [Architecture Overview](#4-architecture-overview)
5. [Technology Stack](#5-technology-stack)
6. [Installation Instructions](#6-installation-instructions)
7. [Build Instructions](#7-build-instructions)
8. [Usage Examples](#8-usage-examples)
9. [Supported Security Rules](#9-supported-security-rules)
10. [Sample Output](#10-sample-output)
11. [Testing Instructions](#11-testing-instructions)
12. [Known Limitations](#12-known-limitations)
13. [Future Improvements](#13-future-improvements)

---

## 1. Introduction

C and C++ remain foundational languages for systems programming, operating systems, embedded devices, and high-performance services. However, manual memory management and legacy C APIs introduce significant security risks. CodeGuard serves as a developer-friendly, extensible static analyzer that integrates into local developer workflows and CI/CD pipelines to catch vulnerabilities early in the software development lifecycle.

---

## 2. Problem Statement

Traditional regex-based or lexical scanning tools struggle to understand the actual syntax, types, and semantics of C/C++ source code. They suffer from:
- **High false-positive rates** (flagging safe code or comments).
- **Inability to reason about buffer capacities and bounds**.
- **Lack of interprocedural awareness** across function boundaries.
- **Blindness to data propagation** from untrusted sources into dangerous sinks.

CodeGuard solves these issues by leveraging the Clang compiler frontend to parse source code into a high-fidelity Abstract Syntax Tree (AST), tracking data flow, and evaluating call relationships.

---

## 3. Features

- **Clang AST Integration**: Full syntactic and semantic inspection using Clang Tooling.
- **Interprocedural Call Analysis**: Tracks function arguments and parameters across calls (e.g., detecting undersized buffers passed into helper copy functions).
- **Basic Data-Flow Taint Tracking**: Traces untrusted input from sources (`cin`, `scanf`, `fgets`, `argv`) to dangerous sinks (`strcpy`, `sprintf`, `system`, `memcpy`).
- **Comprehensive Rule Suite**: 10 built-in security rules covering string operations, memory management, integer arithmetic, command execution, and pointer dereferencing.
- **False Positive Reduction**: Safe-case reasoning evaluates constant buffer capacities against source string lengths and detects null-checks/reassignments.
- **Multi-Format Reporting**: Generates formatted, human-readable terminal reports and machine-readable JSON reports.
- **Command-Line Interface**: Supports directory and file scanning, severity filtering (`HIGH`, `MEDIUM`, `LOW`), output file redirection, `--help`, and `--version`.
- **CI/CD Ready**: Automated GitHub Actions workflow with exit codes reflecting security policy enforcement.

---

## 4. Architecture Overview

```
CodeGuard/
├── CMakeLists.txt           # Build configuration (C++17, Clang/LLVM, GoogleTest)
├── README.md                # Main documentation
├── docs/
│   ├── architecture.md      # Detailed system architecture
│   ├── security-rules.md    # Catalog of all security rules & remediations
│   └── development.md       # Contributor and developer guide
├── examples/
│   ├── ast_test_cases.cpp   # Comprehensive positive and safe test cases
│   └── vulnerable.cpp       # Demonstration codebase
├── src/
│   ├── main.cpp             # CLI orchestrator & execution entry point
│   ├── scanner/             # Recursive directory scanner
│   ├── lexer/               # Tokenizer and comment stripper
│   ├── ast/                 # Clang AST analyzer & visitor engine
│   ├── analyzer/            # Core analysis abstractions
│   │   ├── callgraph/       # Call-graph & interprocedural parameter tracking
│   │   └── dataflow/        # Source-to-sink taint tracking
│   ├── rules/               # Token-based and AST security rules
│   ├── reporter/            # Terminal & JSON report generators
│   └── cli/                 # POSIX command-line option parser
├── tests/                   # GoogleTest unit & regression suites
└── .github/
    └── workflows/
        └── security.yml     # Automated CI/CD workflow
```

For more details, see [docs/architecture.md](docs/architecture.md).

---

## 5. Technology Stack

- **Language:** Modern C++ (C++17 Standard)
- **Compiler Infrastructure:** Clang / LLVM (LibTooling, Frontend, AST)
- **Build System:** CMake 3.16+
- **Testing Framework:** GoogleTest (GTest) & CTest
- **Continuous Integration:** GitHub Actions

---

## 6. Installation Instructions

### Dependencies (Ubuntu / Debian):

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build clang llvm libclang-dev llvm-dev libgtest-dev
```

---

## 7. Build Instructions

Clone the repository and build using CMake:

```bash
git clone https://github.com/your-username/CodeGuard.git
cd CodeGuard

# Configure CMake
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Build executable and test suites
cmake --build build -j$(nproc)
```

The compiled binary will be located at `./build/codeguard`.

---

## 8. Usage Examples

### Basic Directory Scanning
```bash
./build/codeguard examples/
```

### Display Help and Version
```bash
./build/codeguard --help
./build/codeguard --version
```

### Generate Machine-Readable JSON Output
```bash
./build/codeguard examples/ --format json --output report.json
```

### Filter by Severity Level
```bash
./build/codeguard examples/ --severity HIGH
```

### Scan a Single Source File
```bash
./build/codeguard examples/vulnerable.cpp
```

---

## 9. Supported Security Rules

CodeGuard implements 10 security rules:

| Rule ID | Rule Name | Severity | Target | Description |
|---|---|---|---|---|
| **STR001** | Buffer Overflow | `HIGH` | `strcpy()` | Detects unbounded copies, destination capacity overflows, and tainted source flows |
| **STR002** | Dangerous Function | `HIGH` | `gets()` | Detects usage of inherently unsafe `gets()` |
| **STR003** | Buffer Overflow | `HIGH` | `sprintf()` | Detects formatted string output that may exceed destination capacity |
| **STR004** | Unterminated String | `HIGH` | `strncpy()` | Detects `strncpy()` calls that leave the destination buffer without a null terminator |
| **MEM001** | Buffer Overflow | `HIGH` | `memcpy()` | Detects byte count copies exceeding destination buffer capacity |
| **MEM002** | Suspicious Bounds | `MEDIUM` | `memmove()` | Detects byte count exceeding source or destination buffer bounds |
| **MEM003** | Use-After-Free | `HIGH` | `free()`, `delete` | Detects dereferencing or reusing pointers after memory deallocation |
| **INT001** | Integer Overflow | `MEDIUM` | `malloc()`, `new[]` | Detects non-constant arithmetic in allocation sizes susceptible to wraparound |
| **CMD001** | Command Injection | `HIGH` | `system()`, `popen()`| Detects dynamic or untrusted data passed to command execution APIs |
| **PTR001** | Null Dereference | `HIGH` | `*ptr`, `ptr->` | Detects dereferencing pointers initialized to null without checks |

Detailed descriptions and remediations are documented in [docs/security-rules.md](docs/security-rules.md).

---

## 10. Sample Output

### 10.1 Terminal Report
```text
========================================
          CODEGUARD SECURITY REPORT
========================================

[HIGH] STR001 - Buffer Overflow (strcpy)

File: examples/vulnerable.cpp
Line: 18
Function: main()

Description:
Potential buffer overflow: destination capacity is 10 bytes, but source string length is 27 bytes.

Recommendation:
Use bounded operations such as strncpy(), strlcpy(), or std::string, and validate bounds.

----------------------------------------

SUMMARY

Files discovered:       2
Files analyzed:         2
Failed analyses:        0
Total findings:         15

HIGH:                   13
MEDIUM:                 2
LOW:                    0

========================================
```

### 10.2 JSON Report
```json
{
  "tool": "CodeGuard",
  "version": "1.0",
  "files_scanned": 2,
  "files_analyzed": 2,
  "failed_analyses": 0,
  "summary": {
    "total_findings": 15,
    "high": 13,
    "medium": 2,
    "low": 0
  },
  "findings": [
    {
      "rule_id": "STR001",
      "severity": "HIGH",
      "file": "examples/vulnerable.cpp",
      "line": 18,
      "function": "main()",
      "message": "Potential buffer overflow: destination capacity is 10 bytes, but source string length is 27 bytes.",
      "remediation": "Use bounded operations such as strncpy(), strlcpy(), or std::string, and validate bounds."
    }
  ]
}
```

---

## 11. Testing Instructions

CodeGuard includes a comprehensive automated test suite consisting of unit tests, AST integration tests, CLI parsing tests, and reporting tests.

### Run All Tests via CTest:
```bash
ctest --test-dir build --output-on-failure
```

### Run GoogleTest Binary Directly:
```bash
./build/codeguard_tests
```

### Run AST Integration Test:
```bash
./build/ast_integration_test
```

---

## 12. Known Limitations

- **Full Pointer Alias Analysis**: Complex multi-level pointer indirection (e.g., `char***`) and aliasing across distant compilation units are not fully modeled in v1.0.
- **Path-Sensitive Symbolic Execution**: CodeGuard does not perform complete SAT/SMT-based path-sensitive constraint solving.
- **Third-Party External Libraries**: In the absence of a compilation database with external header paths, header definitions outside standard search paths fall back to syntax-only analysis.
- **Dynamic Polymorphism**: Virtual function dispatch resolutions are analyzed based on static declared types.

---

## 13. Future Improvements

- **Full Interprocedural Call Graph Traversal**: Support whole-program call-graph linking across multiple translation units.
- **Path-Sensitive Taint Analysis**: Integration with Clang Static Analyzer's Symbolic Execution engine (`clang::ento::ExprEngine`).
- **SARIF Report Format**: Support OASIS SARIF (Static Analysis Results Interchange Format) for native GitHub Security Code Scanning tab integration.
- **Automated Fix-It Hints**: Integration with Clang `FixItHint` to automatically suggest and apply secure code refactorings.
- **Concurrency & Thread Safety Rules**: Rules targeting data races, unsafe mutex locking, and double-locking.
