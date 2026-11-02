#include "analyzer.h"
#include <iostream>

using namespace std;

vector<Finding> analyzeFile(
    const string& filePath,
    const vector<Rule*>& rules
) {
    vector<Finding> findings;

    // Convert source code into tokens
    vector<Token> tokens = tokenizeFile(filePath);

    // Run every security rule on the tokens
    for (Rule* rule : rules) {

        vector<Finding> currentFindings =
            rule->check(tokens, filePath);

        findings.insert(
            findings.end(),
            currentFindings.begin(),
            currentFindings.end()
        );
    }

    return findings;
}