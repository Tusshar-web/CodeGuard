#ifndef GETS_RULE_H
#define GETS_RULE_H

#include "rule.h"

class GetsRule : public Rule
{
public:
    vector<Finding> check(
        const vector<Token> &tokens,
        const string &filePath) override;
};

#endif