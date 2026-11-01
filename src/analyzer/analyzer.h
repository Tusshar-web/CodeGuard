#ifndef ANALYZER_H
#define ANALYZER_H

#include <string>
#include <vector>
#include "../rules/rule.h"

using namespace std;

void analyzeFile(const string& filePath, const vector<Rule*>& rules);

#endif