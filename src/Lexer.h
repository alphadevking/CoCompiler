#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include "Tokens.h" // Includes TokenType and Token struct

class Lexer {
private:
    const std::string& source; // Reference to the source code string
    int current_pos;           // Current position in the source string
    int current_line;          // Current line number for error reporting
    int current_column;        // Current column number for error reporting

    char peek();        // Look at the current character without advancing
    char advance();     // Get the current character and move to the next
    bool isAtEnd();     // Check if we've reached the end of the source

    // Helper functions for identifying character types
    bool isDigit(char c);
    bool isAlpha(char c); // For future identifiers/keywords
    bool isAlphaNumeric(char c); // For future identifiers/keywords

    void skipWhitespace();
    Token number(); // Reads a number literal
    Token string_literal(); // New: Reads a string literal

    std::vector<std::string> errors; // Collect lexer errors

public:
    Lexer(const std::string& source_code);

    std::vector<Token> tokenize(); // Main function to produce all tokens
};

#endif // LEXER_H
