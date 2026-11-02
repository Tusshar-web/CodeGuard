#ifndef STRCPY_RULE_H
#define STRCPY_RULE_H

#include "rule.h"

class StrcpyRule : public Rule
{
public:
    vector<Finding> check(
        const vector<Token> &tokens,
        const string &filePath) override;
};

#endif