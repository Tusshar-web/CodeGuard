#ifndef RULE_H
#define RULE_H

#include <string>
#include <vector>

#include "../analyzer/finding.h"
#include "../lexer/lexer.h"

using namespace std;

class Rule {
public:
    virtual vector<Finding> check(
        const vector<Token>& tokens,
        const string& filePath
    ) = 0;

    virtual ~Rule() = default;
};

#endif