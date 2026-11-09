#ifndef CALL_GRAPH_H
#define CALL_GRAPH_H

#include <string>
#include <vector>
#include <unordered_map>
#include "../finding.h"

struct ParameterInfo {
    std::string name;
    std::string type;
    int index = 0;
};

struct CallSite {
    std::string callerName;
    std::string calleeName;
    std::string filePath;
    int lineNumber = 0;
    std::vector<std::string> argumentExpressions;
    std::vector<long long> knownArgSizes; // buffer capacity or string length (-1 if unknown)
    std::vector<bool> isStringLiteral;
    std::vector<std::string> stringLiteralValues;
};

struct ParameterCopyOp {
    int destParamIndex = -1;
    int srcParamIndex = -1;
    std::string opName; // "strcpy", "memcpy", etc.
    int line = 0;
};

struct FunctionSummary {
    std::string name;
    std::string returnType;
    bool isDefinition = false;
    std::string filePath;
    int definitionLine = 0;
    std::vector<ParameterInfo> parameters;
    std::vector<CallSite> outgoingCalls;
    std::vector<ParameterCopyOp> paramCopyOps;
};

class CallGraph {
public:
    void addFunctionSummary(const FunctionSummary& summary);
    void addCallSite(const CallSite& callSite);

    const std::unordered_map<std::string, FunctionSummary>& getFunctions() const {
        return functions_;
    }

    const std::vector<CallSite>& getAllCallSites() const {
        return allCallSites_;
    }

    // Interprocedural analysis: inspect callers and detect cross-function buffer overflows
    std::vector<Finding> analyzeInterproceduralVulnerabilities() const;

private:
    std::unordered_map<std::string, FunctionSummary> functions_;
    std::vector<CallSite> allCallSites_;
};

#endif

