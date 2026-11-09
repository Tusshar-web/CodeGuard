#include <gtest/gtest.h>

#include "../src/rules/strcpy_rule.h"
#include "../src/rules/gets_rule.h"
#include "../src/rules/sprintf_rule.h"
#include "../src/lexer/lexer.h"
#include <fstream>
#include <filesystem>

using namespace std;

// Test STR001: strcpy()

TEST(StrcpyRuleTest, DetectsStrcpy) {

    StrcpyRule rule;

    vector<Token> tokens = {
        {"strcpy", 10, TokenType::Identifier},
        {"(", 10, TokenType::Punctuation}
    };

    vector<Finding> findings = rule.check(tokens, "test.cpp");

    EXPECT_EQ(findings.size(), 1);
    EXPECT_EQ(findings[0].ruleName, "STR001");
    EXPECT_EQ(findings[0].lineNumber, 10);
}

TEST(StrcpyRuleTest, IgnoresSimilarIdentifier) {

    StrcpyRule rule;

    vector<Token> tokens = {
        {"my_strcpy", 5, TokenType::Identifier},
        {"(", 5, TokenType::Punctuation}
    };

    vector<Finding> findings = rule.check(tokens, "test.cpp");

    EXPECT_TRUE(findings.empty());
}


// Test STR002: gets()

TEST(GetsRuleTest, DetectsGets) {

    GetsRule rule;

    vector<Token> tokens = {
        {"gets", 15, TokenType::Identifier},
        {"(", 15, TokenType::Punctuation}
    };

    vector<Finding> findings = rule.check(tokens, "test.cpp");

    EXPECT_EQ(findings.size(), 1);
    EXPECT_EQ(findings[0].ruleName, "STR002");
    EXPECT_EQ(findings[0].lineNumber, 15);
}


// Test STR003: sprintf()

TEST(SprintfRuleTest, DetectsSprintf) {

    SprintfRule rule;

    vector<Token> tokens = {
        {"sprintf", 20, TokenType::Identifier},
        {"(", 20, TokenType::Punctuation}
    };

    vector<Finding> findings = rule.check(tokens, "test.cpp");

    EXPECT_EQ(findings.size(), 1);
    EXPECT_EQ(findings[0].ruleName, "STR003");
    EXPECT_EQ(findings[0].lineNumber, 20);
}

TEST(LexerTest, IgnoresCommentsAndStrings) {

    string filePath =
        (filesystem::temp_directory_path() /
         "codeguard_lexer_test.cpp").string();

    ofstream file(filePath);

    file << R"CODE(
strcpy(buffer, source);

// gets(input);

const char* text = "sprintf(buffer, format)";

/*
strcpy(a, b);
*/

my_strcpy(buffer, source);
)CODE";

    file.close();

    vector<Token> tokens = tokenizeFile(filePath);

    int strcpyCount = 0;
    int getsCount = 0;
    int sprintfCount = 0;
    int similarIdentifierCount = 0;

    for (const auto& token : tokens) {

        if (token.type != TokenType::Identifier)
            continue;

        if (token.value == "strcpy")
            strcpyCount++;

        if (token.value == "gets")
            getsCount++;

        if (token.value == "sprintf")
            sprintfCount++;

        if (token.value == "my_strcpy")
            similarIdentifierCount++;
    }

    EXPECT_EQ(strcpyCount, 1);
    EXPECT_EQ(getsCount, 0);
    EXPECT_EQ(sprintfCount, 0);
    EXPECT_EQ(similarIdentifierCount, 1);

    filesystem::remove(filePath);
}