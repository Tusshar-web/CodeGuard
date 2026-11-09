#ifndef AST_ANALYZER_H
#define AST_ANALYZER_H

#include <string>
#include <vector>

#include "../analyzer/finding.h"
#include "../analyzer/callgraph/call_graph.h"
#include "../analyzer/dataflow/data_flow.h"

// Analyzes a single C++ source file using Clang AST and returns all security findings
bool analyzeFileWithAST(
    const std::string& filePath,
    std::vector<Finding>& findings
);

// Analyzes a single file and also returns CallGraph and DataFlow summaries for advanced inspection
bool analyzeFileWithASTDetailed(
    const std::string& filePath,
    std::vector<Finding>& findings,
    CallGraph& callGraph,
    DataFlowTracker& dataFlow
);

#endif
