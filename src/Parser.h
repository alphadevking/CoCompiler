#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include "../include/Tokens.h"
#include "../include/AST.h"

class Parser {
private:
    const std::vector<Token>& tokens;
    int current_pos;

    Token peek();
    Token advance();
    bool match(TokenType type);
    Token consume(TokenType type, const std::string& message);

    // Parsing functions for expressions
    Expression* primary();
    Expression* unary(); // New: For future unary operators like -x
    Expression* factor();
    Expression* term();
    Expression* comparison(); // New: For comparison operators (>, <, >=, <=, ==, !=)
    Expression* logical_and(); // New: For logical AND (&&)
    Expression* logical_or(); // New: For logical OR (||)
    Expression* assignment(); // New: For assignment expressions
    Expression* expression(); // Top-level expression parsing

    // Parsing functions for statements
    ASTNode* parseStatement(); // New: Handles statements like var x = 10; or expressions
    ASTNode* parseVariableDeclaration(); // New: Parses 'var identifier = expression;'
    Expression* parseIdentifier(); // New: Parses an identifier reference

    // Parsing functions for control flow
    ASTNode* parseIfStatement(); // New: Parses 'if (condition) { ... } else { ... }'
    ASTNode* block(); // New: Parses a block of statements enclosed in {}

    // Parsing functions for other statements
    ASTNode* parsePrintStatement(); // New: Parses 'print expression;'

public:
    Parser(const std::vector<Token>& tokens);
    ASTNode* parse(); // Changed return type to ASTNode* to accommodate statements
};

#endif // PARSER_H
