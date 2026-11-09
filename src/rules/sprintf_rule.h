#ifndef SPRINTF_RULE_H
#define SPRINTF_RULE_H

#include "rule.h"

class SprintfRule : public Rule {
public:
    vector<Finding> check(
        const vector<Token>& tokens,
        const string& filePath
    ) override;
};

#endif