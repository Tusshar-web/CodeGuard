#include "analyzer.h"
#include <iostream>
#include <fstream>

using namespace std;

void analyzeFile(const string& filePath, const vector<Rule*>& rules) {
    ifstream file(filePath);

    if (!file.is_open()) {
        cerr << "Could not open file: " << filePath << '\n';
        return;
    }

    string line;
    int lineNumber = 0;

    while (getline(file, line)) {
        lineNumber++;

        for (Rule* rule : rules) {
            rule->check(line, lineNumber, filePath);
        }
    }
}