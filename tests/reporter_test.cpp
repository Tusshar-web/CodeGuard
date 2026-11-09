#include <gtest/gtest.h>
#include "../src/reporter/terminal_reporter.h"
#include "../src/reporter/json_reporter.h"

TEST(ReporterTest, TerminalReportContainsHeadersAndFindings) {
    TerminalReporter reporter;
    std::vector<Finding> findings = {
        {
            "examples/vulnerable.cpp",
            18,
            "STR001",
            "HIGH",
            "Potentially unsafe use of strcpy()",
            "copyData",
            "Use an appropriately bounded operation"
        }
    };

    ScanStatistics stats = ScanStatistics::compute(1, 1, 0, findings);
    std::string output = reporter.renderToString(findings, stats);

    EXPECT_NE(output.find("CODEGUARD SECURITY REPORT"), std::string::npos);
    EXPECT_NE(output.find("[HIGH] STR001"), std::string::npos);
    EXPECT_NE(output.find("File: examples/vulnerable.cpp"), std::string::npos);
    EXPECT_NE(output.find("Line: 18"), std::string::npos);
    EXPECT_NE(output.find("Function: copyData()"), std::string::npos);
    EXPECT_NE(output.find("SUMMARY"), std::string::npos);
    EXPECT_NE(output.find("Total findings:         1"), std::string::npos);
    EXPECT_NE(output.find("HIGH:                   1"), std::string::npos);
}

TEST(ReporterTest, JsonReportValidStructure) {
    JsonReporter reporter;
    std::vector<Finding> findings = {
        {
            "examples/vulnerable.cpp",
            18,
            "STR001",
            "HIGH",
            "Potentially unsafe use of strcpy()",
            "copyData",
            "Use bounded operation"
        }
    };

    ScanStatistics stats = ScanStatistics::compute(1, 1, 0, findings);
    std::string json = reporter.renderToString(findings, stats);

    EXPECT_NE(json.find("\"tool\": \"CodeGuard\""), std::string::npos);
    EXPECT_NE(json.find("\"version\": \"1.0\""), std::string::npos);
    EXPECT_NE(json.find("\"files_scanned\": 1"), std::string::npos);
    EXPECT_NE(json.find("\"rule_id\": \"STR001\""), std::string::npos);
    EXPECT_NE(json.find("\"severity\": \"HIGH\""), std::string::npos);
    EXPECT_NE(json.find("\"line\": 18"), std::string::npos);
    EXPECT_NE(json.find("\"high\": 1"), std::string::npos);
}

TEST(ReporterTest, JsonReportEscaping) {
    JsonReporter reporter;
    std::vector<Finding> findings = {
        {
            "path/with\"quotes\"\\and\\slashes.cpp",
            5,
            "CMD001",
            "HIGH",
            "Message with \"quotes\" and\nnewlines",
            "main",
            "Remediation with \"quotes\""
        }
    };

    ScanStatistics stats = ScanStatistics::compute(1, 1, 0, findings);
    std::string json = reporter.renderToString(findings, stats);

    EXPECT_NE(json.find("\\\"quotes\\\""), std::string::npos);
    EXPECT_NE(json.find("\\nnewlines"), std::string::npos);
}

