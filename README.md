# CodeGuard 🛡️

**CodeGuard** is a lightweight, token-based Static Application Security Testing (SAST) tool designed for C and C++ codebases. It scans source trees, tokenizes code while filtering out comments and literals, and runs modular security rules to detect dangerous functions, buffer overflow vectors, and common security pitfalls.

---

## 📑 Table of Contents

- [Features](#-features)
- [How It Works](#-how-it-works)
- [Project Architecture](#-project-architecture)
- [Security Rules](#-security-rules)
- [Prerequisites](#-prerequisites)
- [Build Instructions](#-build-instructions)
- [Usage & Example](#-usage--example)
- [Extending CodeGuard (Adding Rules)](#-extending-codeguard-adding-rules)
- [Roadmap](#-roadmap)
- [Contributing](#-contributing)
- [License](#-license)

---

## ✨ Features

- **Recursive Directory Scanner**: Automatically discovers C and C++ source and header files (`.cpp`, `.c`, `.h`, `.hpp`) across nested directories.
- **Lexical Tokenizer**: Robust custom lexer that strips single-line (`//`) and multi-line (`/* ... */`) comments, string literals (`"..."`), and character literals (`'...'`) while preserving accurate source line tracking to eliminate false positives inside comments and strings.
- **Extensible Rule Engine**: Clean, object-oriented rule architecture (`Rule` base class) allowing developers to add custom security checks with minimal code.
- **Clear Severity & Reporting**: Detailed console diagnostics displaying severity level, Rule ID, file path, line number, and actionable remediation messages.
- **Zero Heavy Dependencies**: Built purely with modern standard C++17 and standard libraries (no external third-party dependencies required).

---

## ⚙️ How It Works

CodeGuard analyzes code in four streamlined stages:

```
┌─────────────────────────┐
│ Target Directory / Code │
└────────────┬────────────┘
             │
             ▼
   [1. Directory Scanner]       Recursively discovers .cpp, .c, .h, .hpp files
             │
             ▼
   [2. Lexer / Tokenizer]       Filters comments & literals, emits structured tokens
             │
             ▼
   [3. Rule Engine]             Evaluates tokens against active security rules
             │
             ▼
   [4. Report Generator]        Aggregates findings & outputs formatted report
```

1. **Scanner**: Scans the user-supplied directory path using `std::filesystem::recursive_directory_iterator`.
2. **Lexer**: Reads source files and generates a linear stream of tokens (`TokenType::Identifier` and `TokenType::Punctuation`) with exact line numbers.
3. **Analyzer**: Feeds token streams through all registered security rules.
4. **Reporter**: Collects vulnerabilities into `Finding` objects and prints a summary report with scan metrics.

---

## 📂 Project Architecture

```
CodeGuard/
├── CMakeLists.txt           # CMake build configuration (C++17)
├── .gitignore               # Git ignore rules for build artifacts and binaries
├── README.md                # Project documentation
├── examples/
│   └── vulnerable.cpp       # Sample code demonstrating detectable vulnerabilities
└── src/
    ├── main.cpp             # CLI entry point, scan orchestration, and report display
    ├── scanner/
    │   ├── scanner.h        # Directory scanner interface
    │   └── scanner.cpp      # Recursive directory traversal implementation
    ├── lexer/
    │   ├── lexer.h          # Token structure and tokenizer declarations
    │   └── lexer.cpp        # Lexical analysis and comment/literal stripping
    ├── analyzer/
    │   ├── analyzer.h       # File analysis coordinator interface
    │   ├── analyzer.cpp     # Tokenization and rule execution pipeline
    │   └── finding.h        # Finding data model (severity, file, line, message)
    ├── rules/
    │   ├── rule.h           # Abstract base class for security rules
    │   ├── strcpy_rule.h    # STR001 (strcpy) rule definition
    │   ├── strcpy_rule.cpp  # STR001 pattern matcher
    │   ├── gets_rule.h      # STR002 (gets) rule definition
    │   └── gets_rule.cpp    # STR002 pattern matcher
    └── reporter/            # Module reserved for multi-format reporters (JSON, SARIF)
```

---

## 🔍 Security Rules

CodeGuard currently implements the following built-in security rules:

| Rule ID | Rule Name | Severity | Target Function | Risk & Recommendation |
| :--- | :--- | :---: | :--- | :--- |
| **STR001** | `StrcpyRule` | `HIGH` | `strcpy()` | Does not check buffer boundaries, leading to potential buffer overflows. Replace with safer alternatives like `strncpy`, `strlcpy`, or modern `std::string`. |
| **STR002** | `GetsRule` | `HIGH` | `gets()` | Highly unsafe function that provides no mechanism to prevent buffer overruns. Removed from C11/C++14 standards. Replace with `fgets`, `getline`, or `std::cin`. |

---

## 🛠️ Prerequisites

To build and run CodeGuard, ensure you have:

- **Compiler**: A C++17 compliant compiler:
  - GCC 9+
  - Clang 10+
  - MSVC 2019+
- **Build System**: CMake 3.16 or newer
- **Make / Ninja** or your preferred native build tool

---

## 🚀 Build Instructions

Clone the repository and build using CMake:

```bash
# 1. Clone the repository
git clone https://github.com/your-username/CodeGuard.git
cd CodeGuard

# 2. Generate build files
cmake -B build -S .

# 3. Compile the executable
cmake --build build
```

The compiled binary `codeguard` will be generated inside the `build/` directory.

---

## 💻 Usage & Example

### Running CodeGuard

Launch the executable from your terminal:

```bash
./build/codeguard
```

When prompted, enter the relative or absolute path of the directory you wish to scan:

```text
Enter directory to scan: examples
```

### Sample Output

Testing CodeGuard on `examples/vulnerable.cpp`:

```text
Enter directory to scan: examples

Scanning started...

========== CODEGUARD REPORT ==========

[HIGH] STR001
File: examples/vulnerable.cpp
Line: 10
Message: Potentially unsafe use of strcpy()

[HIGH] STR002
File: examples/vulnerable.cpp
Line: 13
Message: Unsafe use of gets()

Total files scanned: 1
Total findings: 2
```

---

## 🧩 Extending CodeGuard (Adding Rules)

Adding a new security check is straightforward thanks to the modular `Rule` base interface.

### 1. Define the Rule Header (`src/rules/sprintf_rule.h`)

```cpp
#ifndef SPRINTF_RULE_H
#define SPRINTF_RULE_H

#include "rule.h"

class SprintfRule : public Rule {
public:
    std::vector<Finding> check(
        const std::vector<Token>& tokens,
        const std::string& filePath
    ) override;
};

#endif
```

### 2. Implement the Rule Logic (`src/rules/sprintf_rule.cpp`)

```cpp
#include "sprintf_rule.h"

std::vector<Finding> SprintfRule::check(
    const std::vector<Token>& tokens,
    const std::string& filePath
) {
    std::vector<Finding> findings;

    for (size_t i = 0; i + 1 < tokens.size(); ++i) {
        if (tokens[i].type == TokenType::Identifier &&
            tokens[i].value == "sprintf" &&
            tokens[i + 1].value == "(") {
            findings.push_back({
                filePath,
                tokens[i].lineNumber,
                "STR003",
                "MEDIUM",
                "Use of sprintf() may lead to buffer overflows; consider snprintf()"
            });
        }
    }
    return findings;
}
```

### 3. Register Rule in `src/main.cpp` and `CMakeLists.txt`

Add your rule to `CMakeLists.txt` under `add_executable`, instantiate it in `main.cpp`, and append it to the `rules` list:

```cpp
#include "rules/sprintf_rule.h"

// Inside main():
SprintfRule sprintfRule;
vector<Rule*> rules = {
    &strcpyRule,
    &getsRule,
    &sprintfRule
};
```

---

## 🗺️ Roadmap

- [ ] **Multi-Format Reports**: Support for JSON, SARIF (Static Analysis Results Interchange Format), and HTML export for CI/CD integration.
- [ ] **CLI Flags & Arguments**: Replace interactive `cin` with POSIX-style CLI arguments (`--path`, `--rule`, `--severity`, `--output`).
- [ ] **Configurable Rule Engine**: Config files (`.codeguard.yaml` or `.json`) to toggle rules and adjust severity thresholds.
- [ ] **Additional Rules**:
  - `system()` command injection detection
  - Format string vulnerabilities (`printf(userInput)`)
  - Memory leak heuristics (`malloc` / `new` without corresponding `free` / `delete`)
  - Banned C functions (`strcat`, `scanf`, etc.)
- [ ] **Automated Testing**: Unit test suite using Google Test (CTest integration in `tests/`).

---

## 🤝 Contributing

Contributions are welcome! Please feel free to open issues or submit pull requests:

1. Fork the Project.
2. Create your Feature Branch (`git checkout -b feature/NewSecurityRule`).
3. Commit your Changes (`git commit -m 'Add new rule for sprintf'`).
4. Push to the Branch (`git push origin feature/NewSecurityRule`).
5. Open a Pull Request.

---

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.
