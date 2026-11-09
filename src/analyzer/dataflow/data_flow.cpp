#include "data_flow.h"
#include <sstream>

void DataFlowTracker::markTainted(const std::string& varName,
                                  const std::string& sourceApi,
                                  const std::string& filePath,
                                  int line) {
    TaintSource source;
    source.variableName = varName;
    source.sourceApi = sourceApi;
    source.filePath = filePath;
    source.line = line;
    taintedVariables_[varName] = source;
}

void DataFlowTracker::propagateTaint(const std::string& destVar,
                                     const std::string& srcVar) {
    auto it = taintedVariables_.find(srcVar);
    if (it != taintedVariables_.end()) {
        TaintSource newSource = it->second;
        newSource.variableName = destVar;
        taintedVariables_[destVar] = newSource;
    }
}

bool DataFlowTracker::isTainted(const std::string& varName) const {
    return taintedVariables_.find(varName) != taintedVariables_.end();
}

const TaintSource* DataFlowTracker::getTaintSource(const std::string& varName) const {
    auto it = taintedVariables_.find(varName);
    if (it != taintedVariables_.end()) {
        return &(it->second);
    }
    return nullptr;
}

void DataFlowTracker::recordSink(const std::string& varName,
                                 const std::string& sinkApi,
                                 const std::string& filePath,
                                 int line,
                                 const std::string& functionName) {
    auto it = taintedVariables_.find(varName);
    if (it != taintedVariables_.end()) {
        TaintFlow flow;
        flow.variableName = varName;
        flow.source = it->second;
        flow.sinkApi = sinkApi;
        flow.filePath = filePath;
        flow.sinkLine = line;
        flow.functionName = functionName;
        detectedFlows_.push_back(flow);
    }
}

std::vector<Finding> DataFlowTracker::generateFindings() const {
    std::vector<Finding> findings;

    for (const auto& flow : detectedFlows_) {
        std::string ruleId;
        std::string severity = "HIGH";
        std::string message;
        std::string remediation;

        std::string srcLoc = flow.source.filePath + ":" + std::to_string(flow.source.line);
        std::string snkLoc = flow.filePath + ":" + std::to_string(flow.sinkLine);

        if (flow.sinkApi == "system" || flow.sinkApi == "popen") {
            ruleId = "CMD001";
            message = "Potential command injection: untrusted input from [" + flow.source.sourceApi +
                      " at line " + std::to_string(flow.source.line) + "] flows directly to " +
                      flow.sinkApi + "() without sanitization.";
            remediation = "Avoid passing untrusted user input directly to command interpreters. Use execve() with argument arrays.";
        } else if (flow.sinkApi == "strcpy") {
            ruleId = "STR001";
            message = "Unbounded source data: untrusted input from [" + flow.source.sourceApi +
                      " at line " + std::to_string(flow.source.line) + "] flows into strcpy() at line " +
                      std::to_string(flow.sinkLine) + " without length validation.";
            remediation = "Validate input length before copying, or use bounds-checked functions like strncpy() or std::string.";
        } else if (flow.sinkApi == "sprintf") {
            ruleId = "STR003";
            message = "Unbounded formatted copy: untrusted input from [" + flow.source.sourceApi +
                      " at line " + std::to_string(flow.source.line) + "] formatted into destination via sprintf() at line " +
                      std::to_string(flow.sinkLine) + ".";
            remediation = "Use snprintf() with destination capacity and validate input string length.";
        } else if (flow.sinkApi == "memcpy") {
            ruleId = "MEM001";
            message = "Untrusted memory copy: untrusted data from [" + flow.source.sourceApi +
                      " at line " + std::to_string(flow.source.line) + "] used in memcpy() at line " +
                      std::to_string(flow.sinkLine) + ".";
            remediation = "Validate source buffer size and byte count before performing memcpy().";
        } else {
            ruleId = "MEM001";
            message = "Tainted data from source [" + flow.source.sourceApi + "] reaches dangerous sink [" + flow.sinkApi + "].";
            remediation = "Sanitize and validate untrusted input before using in sink operations.";
        }

        findings.push_back({
            flow.filePath,
            flow.sinkLine,
            ruleId,
            severity,
            message,
            flow.functionName,
            remediation,
            srcLoc,
            snkLoc
        });
    }

    return findings;
}

void DataFlowTracker::clear() {
    taintedVariables_.clear();
    detectedFlows_.clear();
}

