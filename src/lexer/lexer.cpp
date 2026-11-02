#include "lexer.h"
#include <fstream>
#include <cctype>

using namespace std;

vector<Token> tokenizeFile(const string& filePath) {

    vector<Token> tokens;

    ifstream file(filePath);

    if (!file.is_open()) {
        return tokens;
    }

    string code(
        (istreambuf_iterator<char>(file)),
        istreambuf_iterator<char>()
    );

    int lineNumber = 1;
    int i = 0;
    int n = code.size();

    while (i < n) {

        // Handle newlines
        if (code[i] == '\n') {
            lineNumber++;
            i++;
            continue;
        }

        // Skip other whitespace
        if (isspace(static_cast<unsigned char>(code[i]))) {
            i++;
            continue;
        }

        // Skip single-line comments
        if (code[i] == '/' && i + 1 < n && code[i + 1] == '/') {

            while (i < n && code[i] != '\n') {
                i++;
            }

            continue;
        }

        // Skip multi-line comments
        if (code[i] == '/' && i + 1 < n && code[i + 1] == '*') {

            i += 2;

            while (i + 1 < n &&
                   !(code[i] == '*' && code[i + 1] == '/')) {

                if (code[i] == '\n') {
                    lineNumber++;
                }

                i++;
            }

            if (i + 1 < n) {
                i += 2;
            }

            continue;
        }

        // Skip string literals
        if (code[i] == '"') {

            i++;

            while (i < n && code[i] != '"') {

                if (code[i] == '\\' && i + 1 < n) {
                    i += 2;
                    continue;
                }

                if (code[i] == '\n') {
                    lineNumber++;
                }

                i++;
            }

            if (i < n) {
                i++;
            }

            continue;
        }

        // Skip character literals
        if (code[i] == '\'') {

            i++;

            while (i < n && code[i] != '\'') {

                if (code[i] == '\\' && i + 1 < n) {
                    i += 2;
                    continue;
                }

                i++;
            }

            if (i < n) {
                i++;
            }

            continue;
        }

        // Identify identifiers
        if (isalpha(static_cast<unsigned char>(code[i])) ||
            code[i] == '_') {

            string identifier;
            int tokenLine = lineNumber;

            while (i < n &&
                   (isalnum(static_cast<unsigned char>(code[i])) ||
                    code[i] == '_')) {

                identifier += code[i];
                i++;
            }

            tokens.push_back({
                identifier,
                tokenLine,
                TokenType::Identifier
            });

            continue;
        }

        // Identify punctuation
        string punctuation(1, code[i]);

        tokens.push_back({
            punctuation,
            lineNumber,
            TokenType::Punctuation
        });

        i++;
    }

    return tokens;
}