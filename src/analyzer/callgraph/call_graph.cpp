#include "call_graph.h"
#include <sstream>

void CallGraph::addFunctionSummary(const FunctionSummary& summary) {
    functions_[summary.name] = summary;
}

void CallGraph::addCallSite(const CallSite& callSite) {
    allCallSites_.push_back(callSite);
}

std::vector<Finding> CallGraph::analyzeInterproceduralVulnerabilities() const {
    std::vector<Finding> findings;

    for (const auto& call : allCallSites_) {
        auto it = functions_.find(call.calleeName);
        if (it == functions_.end()) {
            continue;
        }

        const FunctionSummary& callee = it->second;

        // Check if callee performs a parameter copy operation (e.g. strcpy(dest, src))
        for (const auto& op : callee.paramCopyOps) {
            if (op.destParamIndex < 0 || op.destParamIndex >= static_cast<int>(call.knownArgSizes.size()) ||
                op.srcParamIndex < 0 || op.srcParamIndex >= static_cast<int>(call.knownArgSizes.size())) {
                continue;
            }

            long long destCapacity = call.knownArgSizes[op.destParamIndex];
            long long srcSize = call.knownArgSizes[op.srcParamIndex];

            // If destination capacity is limited and source size exceeds it:
            if (destCapacity > 0 && srcSize > 0 && srcSize > destCapacity) {
                std::ostringstream msg;
                msg << "Interprocedural buffer overflow: " << callee.name << "() performs " << op.opName
                    << " into destination buffer of capacity " << destCapacity
                    << " bytes, but source argument size is " << srcSize << " bytes.";

                std::string functionContext = callee.name + "() called from " + call.callerName;
                std::string remediation = "Ensure destination buffer capacity matches or exceeds source size, "
                                          "or pass buffer capacity to " + callee.name + "() and use bounded operations.";

                findings.push_back({
                    call.filePath,
                    call.lineNumber,
                    "STR001",
                    "HIGH",
                    msg.str(),
                    functionContext,
                    remediation
                });
            }
        }
    }

    return findings;
}

