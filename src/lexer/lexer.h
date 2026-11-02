#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

using namespace std;

enum class TokenType {
    Identifier,
    Punctuation
};

struct Token {
    string value;
    int lineNumber;
    TokenType type;
};

vector<Token> tokenizeFile(const string& filePath);

#endif