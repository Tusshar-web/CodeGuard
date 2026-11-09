# CodeGuard Developer Guide

This guide explains how to set up the development environment, build the project, run tests, and add new security rules to CodeGuard.

---

## 1. Prerequisites

- **OS:** Ubuntu 20.04+ / Debian 11+ / Linux
- **Compiler:** Clang 14+ or GCC 9+ with C++17 support
- **Build System:** CMake 3.16+ and Ninja or Make
- **Libraries:**
  - LLVM & Clang development packages (`libclang-dev`, `llvm-dev`)
  - GoogleTest (`libgtest-dev`)

### Installation on Ubuntu / Debian:

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build clang llvm libclang-dev llvm-dev libgtest-dev
```

---

## 2. Building CodeGuard

### Debug Build:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

### Release Build:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

---

## 3. Running Automated Tests

CodeGuard uses CTest and GoogleTest to verify all security rules, AST operations, reporters, CLI options, and data-flow tracking.

Run the test suite:
```bash
ctest --test-dir build --output-on-failure
```

To run individual test binaries directly:
```bash
./build/codeguard_tests
./build/ast_integration_test
```

---

## 4. How to Add a New Security Rule

CodeGuard's AST analyzer is modular. To add a new rule:

1. **Define the Rule Metadata:**
   Assign a unique Rule ID (e.g. `MEM004`), severity (`HIGH`, `MEDIUM`, `LOW`), clear error message, and remediation recommendation.

2. **Implement Detection in `SecurityVisitor` (`src/ast/ast_analyzer.cpp`):**
   - Override the corresponding AST visit method (e.g. `VisitCallExpr`, `VisitBinaryOperator`, `VisitVarDecl`, or `TraverseFunctionDecl`).
   - Use AST node inspection to check target function name, argument expressions, and types.
   - Implement safe-case checks to avoid false positives.
   - Call `addFinding(...)`.

3. **Add Rule Title in `TerminalReporter` (`src/reporter/terminal_reporter.cpp`):**
   Add your Rule ID to `getRuleTitle(const std::string& ruleId)`.

4. **Add Positive & Safe-Case Tests:**
   - Add test cases to `examples/ast_test_cases.cpp`.
   - Update `src/ast/ast_integration_test.cpp` to verify detection.
   - Add unit tests in `tests/`.

5. **Document the Rule:**
   Add entry to `docs/security-rules.md` and `README.md`.

