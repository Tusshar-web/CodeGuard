#ifndef DATA_FLOW_H
#define DATA_FLOW_H

#include <string>
#include <vector>
#include <unordered_map>
#include "../finding.h"

struct TaintSource {
    std::string variableName;
    std::string sourceApi;    // "cin", "scanf", "fgets", "argv", "read", "getenv"
    std::string filePath;
    int line = 0;
};

struct TaintFlow {
    std::string variableName;
    TaintSource source;
    std::string sinkApi;      // "strcpy", "system", "sprintf", "memcpy", "popen"
    std::string filePath;
    int sinkLine = 0;
    std::string functionName;
    std::string description;
};

class DataFlowTracker {
public:
    void markTainted(const std::string& varName, const std::string& sourceApi, const std::string& filePath, int line);
    void propagateTaint(const std::string& destVar, const std::string& srcVar);
    bool isTainted(const std::string& varName) const;
    const TaintSource* getTaintSource(const std::string& varName) const;

    void recordSink(const std::string& varName,
                    const std::string& sinkApi,
                    const std::string& filePath,
                    int line,
                    const std::string& functionName);

    std::vector<Finding> generateFindings() const;
    void clear();

private:
    std::unordered_map<std::string, TaintSource> taintedVariables_;
    std::vector<TaintFlow> detectedFlows_;
};

#endif

