#include "cli_parser.h"
#include <iostream>
#include <algorithm>

namespace {
std::string toUpper(std::string s) {
    for (char &c : s) {
        c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
    }
    return s;
}
}

CliOptions CliParser::parse(int argc, char* argv[]) {
    CliOptions options;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            options.showHelp = true;
            return options;
        } else if (arg == "--version" || arg == "-v") {
            options.showVersion = true;
            return options;
        } else if (arg == "--format" || arg == "-f") {
            if (i + 1 < argc) {
                std::string fmt = argv[++i];
                std::string lowerFmt = fmt;
                for (char &c : lowerFmt) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
                if (lowerFmt != "text" && lowerFmt != "json") {
                    options.valid = false;
                    options.errorMessage = "Unknown output format: " + fmt + " (supported: text, json)";
                    return options;
                }
                options.format = lowerFmt;
            } else {
                options.valid = false;
                options.errorMessage = "Missing argument for " + arg;
                return options;
            }
        } else if (arg == "--severity" || arg == "-s") {
            if (i + 1 < argc) {
                std::string sev = toUpper(argv[++i]);
                if (sev != "HIGH" && sev != "MEDIUM" && sev != "LOW" && sev != "ALL") {
                    options.valid = false;
                    options.errorMessage = "Unknown severity level: " + sev + " (supported: HIGH, MEDIUM, LOW, ALL)";
                    return options;
                }
                if (sev == "ALL") {
                    options.severityFilter = "";
                } else {
                    options.severityFilter = sev;
                }
            } else {
                options.valid = false;
                options.errorMessage = "Missing argument for " + arg;
                return options;
            }
        } else if (arg == "--output" || arg == "-o") {
            if (i + 1 < argc) {
                options.outputFile = argv[++i];
            } else {
                options.valid = false;
                options.errorMessage = "Missing argument for " + arg;
                return options;
            }
        } else if (!arg.empty() && arg[0] == '-') {
            options.valid = false;
            options.errorMessage = "Unrecognized option: " + arg;
            return options;
        } else {
            if (options.targetPath.empty()) {
                options.targetPath = arg;
            } else {
                options.valid = false;
                options.errorMessage = "Multiple target paths provided: " + options.targetPath + " and " + arg;
                return options;
            }
        }
    }

    return options;
}

void CliParser::printHelp() {
    std::cout << "CodeGuard - C++ Static Security Analyzer (v1.0)\n\n"
              << "USAGE:\n"
              << "  codeguard [OPTIONS] <path>\n\n"
              << "ARGUMENTS:\n"
              << "  <path>                 Directory or source file to scan\n\n"
              << "OPTIONS:\n"
              << "  -h, --help             Display this help message and exit\n"
              << "  -v, --version          Display version information and exit\n"
              << "  -f, --format <format>  Select output format: 'text' (default) or 'json'\n"
              << "  -s, --severity <level> Filter findings by severity: HIGH, MEDIUM, LOW, or ALL\n"
              << "  -o, --output <file>    Write report to specified output file\n\n"
              << "SUPPORTED SECURITY RULES:\n"
              << "  STR001 [HIGH]    Unsafe strcpy() buffer overflow\n"
              << "  STR002 [HIGH]    Unsafe gets() function usage\n"
              << "  STR003 [HIGH]    Unsafe sprintf() buffer overflow\n"
              << "  STR004 [HIGH]    Unsafe strncpy() missing null terminator\n"
              << "  MEM001 [HIGH]    Unsafe memcpy() buffer overflow\n"
              << "  MEM002 [MEDIUM]  Unsafe memmove() buffer bounds\n"
              << "  MEM003 [HIGH]    Potential use-after-free\n"
              << "  INT001 [MEDIUM]  Integer overflow in memory allocation/arithmetic\n"
              << "  CMD001 [HIGH]    Command injection via system() / popen()\n"
              << "  PTR001 [HIGH]    Potential null pointer dereference\n\n"
              << "EXAMPLES:\n"
              << "  codeguard examples/\n"
              << "  codeguard examples/ --format json --output report.json\n"
              << "  codeguard src/ --severity HIGH\n";
}

void CliParser::printVersion() {
    std::cout << "CodeGuard version 1.0.0 (C++ Static Security Analyzer)\n"
              << "Built with LLVM/Clang AST & C++17\n";
}

