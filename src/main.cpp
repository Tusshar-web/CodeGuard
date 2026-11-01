#include <bits/stdc++.h>

#include "scanner/scanner.h"
#include "analyzer/analyzer.h"
#include "rules/strcpy_rule.h"

using namespace std;

int main() {
    string path;

    cout << "Enter directory to scan: ";
    getline(cin, path);

    vector<string> files = scanDirectory(path);

    StrcpyRule strcpyRule;

    vector<Rule*> rules = {
        &strcpyRule
    };

    cout << "\nScanning started...\n";

    for (const auto& file : files) {
        analyzeFile(file, rules);
    }

    cout << "\nScan completed!\n";
    cout << "Total files scanned: " << files.size() << '\n';

    return 0;
}