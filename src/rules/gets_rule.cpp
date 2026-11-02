#include "gets_rule.h"

using namespace std;

vector<Finding> GetsRule::check(
    const vector<Token>& tokens,
    const string& filePath
) {
    vector<Finding> findings;

    for (int i = 0; i + 1 < tokens.size(); i++) {

        if (tokens[i].type == TokenType::Identifier &&
            tokens[i].value == "gets" &&
            tokens[i + 1].value == "(") {

            findings.push_back({
                filePath,
                tokens[i].lineNumber,
                "STR002",
                "HIGH",
                "Unsafe use of gets()"
            });
        }
    }

    return findings;
}