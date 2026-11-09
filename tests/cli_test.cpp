#include <gtest/gtest.h>
#include "../src/cli/cli_parser.h"

TEST(CliParserTest, ParsesHelpFlag) {
    char* argv[] = {(char*)"codeguard", (char*)"--help"};
    CliOptions opt = CliParser::parse(2, argv);
    EXPECT_TRUE(opt.showHelp);
}

TEST(CliParserTest, ParsesShortHelpFlag) {
    char* argv[] = {(char*)"codeguard", (char*)"-h"};
    CliOptions opt = CliParser::parse(2, argv);
    EXPECT_TRUE(opt.showHelp);
}

TEST(CliParserTest, ParsesVersionFlag) {
    char* argv[] = {(char*)"codeguard", (char*)"--version"};
    CliOptions opt = CliParser::parse(2, argv);
    EXPECT_TRUE(opt.showVersion);
}

TEST(CliParserTest, ParsesTargetPath) {
    char* argv[] = {(char*)"codeguard", (char*)"examples/"};
    CliOptions opt = CliParser::parse(2, argv);
    EXPECT_TRUE(opt.valid);
    EXPECT_EQ(opt.targetPath, "examples/");
    EXPECT_EQ(opt.format, "text");
    EXPECT_EQ(opt.severityFilter, "");
}

TEST(CliParserTest, ParsesFormatJson) {
    char* argv[] = {(char*)"codeguard", (char*)"examples/", (char*)"--format", (char*)"json"};
    CliOptions opt = CliParser::parse(4, argv);
    EXPECT_TRUE(opt.valid);
    EXPECT_EQ(opt.format, "json");
}

TEST(CliParserTest, ParsesSeverityFilter) {
    char* argv[] = {(char*)"codeguard", (char*)"examples/", (char*)"--severity", (char*)"HIGH"};
    CliOptions opt = CliParser::parse(4, argv);
    EXPECT_TRUE(opt.valid);
    EXPECT_EQ(opt.severityFilter, "HIGH");
}

TEST(CliParserTest, ParsesOutputFile) {
    char* argv[] = {(char*)"codeguard", (char*)"examples/", (char*)"--output", (char*)"report.json"};
    CliOptions opt = CliParser::parse(4, argv);
    EXPECT_TRUE(opt.valid);
    EXPECT_EQ(opt.outputFile, "report.json");
}

TEST(CliParserTest, RejectsUnknownOption) {
    char* argv[] = {(char*)"codeguard", (char*)"--invalid-opt"};
    CliOptions opt = CliParser::parse(2, argv);
    EXPECT_FALSE(opt.valid);
}

TEST(CliParserTest, RejectsInvalidFormat) {
    char* argv[] = {(char*)"codeguard", (char*)"examples/", (char*)"--format", (char*)"xml"};
    CliOptions opt = CliParser::parse(4, argv);
    EXPECT_FALSE(opt.valid);
}

