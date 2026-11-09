#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <memory>

#include "cli/cli_parser.h"
#include "scanner/scanner.h"
#include "ast/ast_analyzer.h"
#include "reporter/terminal_reporter.h"
#include "reporter/json_reporter.h"

using namespace std;
namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    CliOptions options = CliParser::parse(argc, argv);

    if (options.showHelp) {
        CliParser::printHelp();
        return 0;
    }

    if (options.showVersion) {
        CliParser::printVersion();
        return 0;
    }

    if (!options.valid) {
        cerr << "Error: " << options.errorMessage << "\n\n";
        CliParser::printHelp();
        return 1;
    }

    string path = options.targetPath;

    // Interactive fallback if no path was provided via command line
    if (path.empty()) {
        cout << "Enter directory or file to scan: ";
        if (!getline(cin, path) || path.empty()) {
            cerr << "Error: No scan path provided.\n";
            return 1;
        }
    }

    if (!fs::exists(path)) {
        cerr << "Error: Path does not exist: " << path << "\n";
        return 1;
    }

    vector<string> files;
    if (fs::is_regular_file(path)) {
        files.push_back(fs::absolute(path).lexically_normal().string());
    } else if (fs::is_directory(path)) {
        files = scanDirectory(path);
    } else {
        cerr << "Error: Invalid path type: " << path << "\n";
        return 1;
    }

    vector<Finding> allFindings;
    size_t successfulScans = 0;
    size_t failedScans = 0;

    for (const auto &file : files) {
        vector<Finding> findings;
        bool success = analyzeFileWithAST(file, findings);

        if (success) {
            successfulScans++;
            allFindings.insert(
                allFindings.end(),
                findings.begin(),
                findings.end()
            );
        } else {
            failedScans++;
            cerr << "Warning: Could not analyze: " << file << '\n';
        }
    }

    // Apply severity filter if requested
    vector<Finding> filteredFindings;
    if (!options.severityFilter.empty()) {
        for (const auto &f : allFindings) {
            if (f.severity == options.severityFilter) {
                filteredFindings.push_back(f);
            }
        }
    } else {
        filteredFindings = allFindings;
    }

    ScanStatistics stats = ScanStatistics::compute(
        files.size(),
        successfulScans,
        failedScans,
        filteredFindings
    );

    unique_ptr<Reporter> reporter;
    if (options.format == "json") {
        reporter = make_unique<JsonReporter>();
    } else {
        reporter = make_unique<TerminalReporter>();
    }

    if (!options.outputFile.empty()) {
        ofstream outFile(options.outputFile);
        if (!outFile) {
            cerr << "Error: Could not open output file: " << options.outputFile << "\n";
            return 1;
        }
        reporter->generateReport(filteredFindings, stats, outFile);
        cout << "Report written to " << options.outputFile << "\n";
    } else {
        reporter->generateReport(filteredFindings, stats, cout);
    }

    // Return 1 if critical/high severity vulnerabilities were found, 0 otherwise
    if (stats.highSeverity > 0) {
        return 1;
    }

    return 0;
}
