# CodeGuard Architecture

CodeGuard is a static application security testing (SAST) tool for C and C++ programs that combines Clang Abstract Syntax Tree (AST) analysis, call-graph tracking, and taint-based data-flow analysis to detect software security vulnerabilities before execution.

---

## 1. System Overview

CodeGuard operates via a multi-stage analysis pipeline:

```
Source Files (.cpp, .cc, .cxx)
             │
             ▼
    [ Directory Scanner ]
   Recursively enumerates source files, filtering out build/cache folders
             │
             ▼
   [ Clang Compilation DB ]
   Loads compile_commands.json or falls back to FixedCompilationDatabase
             │
             ▼
   [ Clang Tool & AST Engine ]
   Parses C++ syntax into a high-fidelity Clang AST (ASTContext, TranslationUnit)
             │
             ├──► [ Call Graph Analyzer ]
             │    Extracts function definitions, call sites, and tracks cross-function parameters
             │
             ├──► [ Data-Flow & Taint Engine ]
             │    Identifies input sources (cin, scanf, argv), propagates taint, and flags dangerous sinks
             │
             └──► [ Security AST Visitor ]
                  Evaluates 10 specialized security rules with safe-case reasoning
             │
             ▼
   [ Deduplication & Aggregation ]
   Consolidates findings, links source-to-sink locations, and computes scan statistics
             │
             ▼
  [ Report Generation (Terminal / JSON) ]
```

---

## 2. Core Subsystems

### 2.1 File Scanner (`src/scanner/`)
- Recursively navigates directory trees using `std::filesystem::recursive_directory_iterator`.
- Ignores build artifacts (`build`, `.git`, `.vscode`, `node_modules`).
- Collects target C/C++ translation units.

### 2.2 AST Analyzer (`src/ast/`)
- Implements Clang Tooling integration (`ClangTool`, `ASTFrontendAction`, `ASTConsumer`, `RecursiveASTVisitor`).
- Utilizes `CompilationDatabase::loadFromDirectory` with automatic fallback to `FixedCompilationDatabase` with `-std=c++17 -w -I. -Iinclude -Isrc`.
- Suppresses internal compiler diagnostic chatter via `IgnoringDiagConsumer`.

### 2.3 Call Graph & Interprocedural Analysis (`src/analyzer/callgraph/`)
- `FunctionSummary`: Captures function declarations, definitions, parameter names, types, and positions.
- `CallSite`: Records caller function, callee function, source location, and evaluated argument expressions.
- Interprocedural vulnerability tracking:
  - When a helper function performs a memory operation (e.g. `strcpy(destParam, srcParam)`), CodeGuard inspects call sites in caller functions.
  - Compares destination capacity passed at caller site with source string length.
  - Emits interprocedural buffer overflow findings with full caller-to-callee context.

### 2.4 Data-Flow & Taint Tracker (`src/analyzer/dataflow/`)
- **Sources**: Tracks untrusted input vectors:
  - `std::cin >> var`
  - `scanf(...)`, `fscanf(...)`
  - `fgets(...)`
  - Command-line arguments (`argv`)
  - POSIX input (`read()`, `recv()`, `getenv()`)
- **Propagation**:
  - Propagates taint across variable assignments (`data = input`) and declarations (`char* p = input`).
- **Sinks**:
  - `strcpy()`, `sprintf()`, `memcpy()`, `system()`, `popen()`.
- **Source-to-Sink Output**:
  - Captures exact source location (file:line) and sink location (file:line) in the security finding.

### 2.5 Security Rules & Safe-Case Reasoning
- Inspects expressions using Clang AST properties (`ConstantArrayType`, `StringLiteral`, `IntegerLiteral`, `UnaryOperator`, `BinaryOperator`, `CXXDeleteExpr`).
- Avoids false positives by evaluating buffer bounds against source length, null checks, and reassignments.

### 2.6 Multi-Format Reporters (`src/reporter/`)
- `TerminalReporter`: Produces structured terminal reports with severity summaries and recommendations.
- `JsonReporter`: Produces valid, machine-readable JSON matching CI/CD specifications.

### 2.7 Command-Line Interface (`src/cli/`)
- Modular POSIX argument parsing with flags (`-h`, `-v`, `-f`, `-s`, `-o`).
- Meaningful process exit codes for CI/CD gates.

