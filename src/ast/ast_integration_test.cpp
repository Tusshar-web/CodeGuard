#include <iostream>
#include <vector>
#include <map>
#include "ast_analyzer.h"

using namespace std;

int main()
{
    vector<Finding> findings;
    string filePath = "examples/ast_test_cases.cpp";

    bool success = analyzeFileWithAST(filePath, findings);

    if (!success)
    {
        cerr << "AST analysis failed for " << filePath << endl;
        return 1;
    }

    map<string, int> detectedRules;
    for (const auto &finding : findings)
    {
        detectedRules[finding.ruleName]++;
    }

    // Required security rules to verify
    vector<string> requiredRules = {
        "STR001", // strcpy buffer overflow / interprocedural
        "STR002", // gets
        "STR003", // sprintf
        "STR004", // strncpy
        "MEM001", // memcpy buffer overflow
        "MEM002", // memmove bounds
        "MEM003", // use-after-free
        "INT001", // integer overflow in allocation
        "CMD001", // command injection
        "PTR001"  // null pointer dereference
    };

    cout << "\n========== AST AUTOMATED TEST (ALL 10 RULES) ==========\n";

    bool passed = true;

    for (const auto &rule : requiredRules)
    {
        int count = detectedRules[rule];
        if (count > 0)
        {
            cout << "[PASS] " << rule << " (detected: " << count << " finding(s))" << endl;
        }
        else
        {
            cout << "[FAIL] " << rule << " was NOT detected in test file!" << endl;
            passed = false;
        }
    }

    cout << "\nTotal AST findings detected: " << findings.size() << endl;

    if (passed)
    {
        cout << "\nAll 10 AST security rules passed successfully!" << endl;
        return 0;
    }

    cout << "\nSome AST security rule tests failed!" << endl;
    return 1;
}
