#include "strcpy_rule.h"

using namespace std;

vector<Finding> StrcpyRule::check(
    const vector<Token>& tokens,
    const string& filePath
) {
    vector<Finding> findings;

    for (int i = 0; i + 1 < tokens.size(); i++) {

        if (tokens[i].type == TokenType::Identifier &&
            tokens[i].value == "strcpy" &&
            tokens[i + 1].value == "(") {

            findings.push_back({
                filePath,
                tokens[i].lineNumber,
                "STR001",
                "HIGH",
                "Potentially unsafe use of strcpy()"
            });
        }
    }

    return findings;
}