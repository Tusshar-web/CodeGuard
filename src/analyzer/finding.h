#ifndef FINDING_H
#define FINDING_H

#include <string>

using namespace std;

struct Finding {
    string filePath;
    int lineNumber;
    string ruleName;
    string severity;
    string message;
};

#endif