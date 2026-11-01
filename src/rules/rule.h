#ifndef RULE_H
#define RULE_H

#include <string>

using namespace std;

class Rule {
public:
    virtual void check(const string& line, int lineNumber,
                       const string& filePath) = 0;

    virtual ~Rule() = default;
};

#endif