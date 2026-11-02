#include <bits/stdc++.h>

#include "scanner/scanner.h"
#include "analyzer/analyzer.h"
#include "rules/strcpy_rule.h"
#include "rules/gets_rule.h"

using namespace std;

int main() {
    string path;

    cout << "Enter directory to scan: ";
    getline(cin, path);

    vector<string> files = scanDirectory(path);

    StrcpyRule strcpyRule;
    GetsRule getsRule;

    vector<Rule*> rules = {
        &strcpyRule,
        &getsRule
    };

    vector<Finding> allFindings;

    cout << "\nScanning started...\n";

    for (const auto& file : files) {
        vector<Finding> findings = analyzeFile(file, rules);

        allFindings.insert(
            allFindings.end(),
            findings.begin(),
            findings.end()
        );
    }

    cout << "\n========== CODEGUARD REPORT ==========\n";

    for (const auto& finding : allFindings) {
        cout << "\n[" << finding.severity << "] "
             << finding.ruleName << '\n';

        cout << "File: " << finding.filePath << '\n';
        cout << "Line: " << finding.lineNumber << '\n';
        cout << "Message: " << finding.message << '\n';
    }

    cout << "\nTotal files scanned: " << files.size() << '\n';
    cout << "Total findings: " << allFindings.size() << '\n';

    return 0;
}