#ifndef STRCPY_RULE_H
#define STRCPY_RULE_H

#include "rule.h"

class StrcpyRule : public Rule {
public:
    void check(const string& line, int lineNumber,
               const string& filePath) override;
};

#endif