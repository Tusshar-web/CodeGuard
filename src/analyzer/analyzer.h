#ifndef ANALYZER_H
#define ANALYZER_H

#include <string>
#include <vector>

#include "../rules/rule.h"
#include "finding.h"

using namespace std;

vector<Finding> analyzeFile(
    const string& filePath,
    const vector<Rule*>& rules
);

#endif