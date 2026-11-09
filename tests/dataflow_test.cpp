#include <gtest/gtest.h>
#include "../src/analyzer/dataflow/data_flow.h"
#include "../src/analyzer/callgraph/call_graph.h"

TEST(DataFlowTest, MarksAndPropagatesTaint) {
    DataFlowTracker tracker;
    tracker.markTainted("userInput", "cin", "main.cpp", 10);
    EXPECT_TRUE(tracker.isTainted("userInput"));
    EXPECT_FALSE(tracker.isTainted("safeData"));

    tracker.propagateTaint("sanitizedPtr", "userInput");
    EXPECT_TRUE(tracker.isTainted("sanitizedPtr"));

    const TaintSource* src = tracker.getTaintSource("sanitizedPtr");
    ASSERT_NE(src, nullptr);
    EXPECT_EQ(src->sourceApi, "cin");
    EXPECT_EQ(src->line, 10);
}

TEST(DataFlowTest, GeneratesSourceToSinkFindings) {
    DataFlowTracker tracker;
    tracker.markTainted("input", "cin", "main.cpp", 15);
    tracker.propagateTaint("data", "input");
    tracker.recordSink("data", "strcpy", "main.cpp", 20, "processInput");

    std::vector<Finding> findings = tracker.generateFindings();
    ASSERT_EQ(findings.size(), 1);
    EXPECT_EQ(findings[0].ruleName, "STR001");
    EXPECT_EQ(findings[0].severity, "HIGH");
    EXPECT_EQ(findings[0].lineNumber, 20);
    EXPECT_EQ(findings[0].sourceLocation, "main.cpp:15");
    EXPECT_EQ(findings[0].sinkLocation, "main.cpp:20");
    EXPECT_NE(findings[0].message.find("cin at line 15"), std::string::npos);
}

TEST(CallGraphTest, DetectsInterproceduralBufferOverflow) {
    CallGraph cg;

    // Callee summary: copyData(dest, src) performs strcpy(dest, src)
    FunctionSummary callee;
    callee.name = "copyData";
    callee.isDefinition = true;
    callee.filePath = "utils.cpp";
    callee.definitionLine = 5;
    callee.parameters = {
        {"destination", "char*", 0},
        {"source", "const char*", 1}
    };
    ParameterCopyOp op;
    op.destParamIndex = 0;
    op.srcParamIndex = 1;
    op.opName = "strcpy";
    op.line = 7;
    callee.paramCopyOps.push_back(op);
    cg.addFunctionSummary(callee);

    // Caller site: main calls copyData(buffer[5], "Hello World"[12])
    CallSite site;
    site.callerName = "main";
    site.calleeName = "copyData";
    site.filePath = "main.cpp";
    site.lineNumber = 15;
    site.knownArgSizes = {5, 12}; // 5 bytes capacity, 12 bytes source length
    cg.addCallSite(site);

    std::vector<Finding> findings = cg.analyzeInterproceduralVulnerabilities();
    ASSERT_EQ(findings.size(), 1);
    EXPECT_EQ(findings[0].ruleName, "STR001");
    EXPECT_EQ(findings[0].severity, "HIGH");
    EXPECT_EQ(findings[0].lineNumber, 15);
    EXPECT_NE(findings[0].message.find("Interprocedural buffer overflow"), std::string::npos);
}

