#ifndef TOKENS_H
#define TOKENS_H

#include <string>
#include <vector>
#include <iostream> // For debug printing

// --- Token Types ---
enum class TokenType {
    // End of File
    EOF_TOKEN,

    // Literals
    INT_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL, // New: For string literals like "hello"
    TRUE,           // New: true keyword for boolean literal
    FALSE,          // New: false keyword for boolean literal

    // Operators
    PLUS,       // +
    MINUS,      // -
    STAR,       // *
    SLASH,      // /
    GREATER,    // >
    LESS,       // <
    GREATER_EQUAL, // >=
    LESS_EQUAL, // <=
    EQUAL_EQUAL, // ==
    BANG_EQUAL, // !=
    BANG,       // !
    AND,        // &&
    OR,         // ||

    // Grouping
    LPAREN,     // (
    RPAREN,     // )

    // Keywords
    VAR,        // var keyword for variable declaration
    PRINT,      // print keyword for output

    // Identifiers
    IDENTIFIER, // e.g., x, myVariable

    // Assignment
    ASSIGN,     // =

    // Delimiters
    SEMICOLON,  // ;

    // Control flow
    IF,         // if keyword
    ELSE,       // else keyword
    LBRACE,     // {
    RBRACE,     // }

    // Future: Other keywords, control flow, etc.
};

// --- Token Structure ---
struct Token {
    TokenType type;
    std::string value; // Lexeme (the actual text)
    int line;          // For error reporting
    int column;        // For error reporting

    // Constructor
    Token(TokenType type, std::string value, int line, int column)
        : type(type), value(std::move(value)), line(line), column(column) {}

    // For easy debugging
    std::string toString() const {
        std::string type_str;
        switch (type) {
            case TokenType::EOF_TOKEN:    type_str = "EOF_TOKEN"; break;
            case TokenType::INT_LITERAL:  type_str = "INT_LITERAL"; break;
            case TokenType::FLOAT_LITERAL: type_str = "FLOAT_LITERAL"; break;
            case TokenType::STRING_LITERAL: type_str = "STRING_LITERAL"; break;
            case TokenType::TRUE:         type_str = "TRUE"; break; // New
            case TokenType::FALSE:        type_str = "FALSE"; break; // New
            case TokenType::PLUS:         type_str = "PLUS"; break;
            case TokenType::MINUS:        type_str = "MINUS"; break;
            case TokenType::STAR:         type_str = "STAR"; break;
            case TokenType::SLASH:        type_str = "SLASH"; break;
            case TokenType::GREATER:      type_str = "GREATER"; break;
            case TokenType::LESS:         type_str = "LESS"; break;
            case TokenType::GREATER_EQUAL: type_str = "GREATER_EQUAL"; break;
            case TokenType::LESS_EQUAL:   type_str = "LESS_EQUAL"; break;
            case TokenType::EQUAL_EQUAL:  type_str = "EQUAL_EQUAL"; break;
            case TokenType::BANG_EQUAL:   type_str = "BANG_EQUAL"; break;
            case TokenType::BANG:         type_str = "BANG"; break;
            case TokenType::AND:          type_str = "AND"; break;
            case TokenType::OR:           type_str = "OR"; break;
            case TokenType::LPAREN:       type_str = "LPAREN"; break;
            case TokenType::RPAREN:       type_str = "RPAREN"; break;
            case TokenType::VAR:          type_str = "VAR"; break;
            case TokenType::PRINT:        type_str = "PRINT"; break;
            case TokenType::IDENTIFIER:   type_str = "IDENTIFIER"; break;
            case TokenType::ASSIGN:       type_str = "ASSIGN"; break;
            case TokenType::SEMICOLON:    type_str = "SEMICOLON"; break;
            case TokenType::IF:           type_str = "IF"; break;
            case TokenType::ELSE:         type_str = "ELSE"; break;
            case TokenType::LBRACE:       type_str = "LBRACE"; break;
            case TokenType::RBRACE:       type_str = "RBRACE"; break;
            // Add more as you define them
            default:                      type_str = "UNKNOWN"; break;
        }
        return "Token(" + type_str + ", \"" + value + "\", L" + std::to_string(line) + ":C" + std::to_string(column) + ")";
    }
};

#endif // TOKENS_H
