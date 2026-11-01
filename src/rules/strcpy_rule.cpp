#include "strcpy_rule.h"
#include <iostream>

using namespace std;

void StrcpyRule::check(const string& line, int lineNumber,
                       const string& filePath) {

    if (line.find("strcpy(") != string::npos) {
        cout << "\n[WARNING] Potentially unsafe function detected!\n";
        cout << "File: " << filePath << '\n';
        cout << "Line: " << lineNumber << '\n';
        cout << "Function: strcpy()\n";
        cout << "Reason: strcpy() does not check destination buffer size.\n";
    }
}