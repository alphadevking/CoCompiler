#ifndef COMPILER_H
#define COMPILER_H

#include <vector>
#include "../include/AST.h"
#include "../include/Tokens.h"
#include "../include/Bytecode.h"
#include "../include/SymbolTable.h" // Include for SymbolTable

class Compiler {
private:
    std::vector<Bytecode> bytecode;
    SymbolTable symbolTable; // Member for managing symbols and scopes
    std::vector<std::string> string_literals; // New: To store string literals

private:
    void compileNode(ASTNode* node);
    ASTNode::Type resolveExpressionType(Expression* expr); // New: Helper to resolve expression types

public:
    Compiler();
    ASTNode::Type getLiteralType(Literal* literal);
    std::vector<Bytecode> compile(ASTNode* ast);
    const std::string& getStringLiteral(int index) const; // New: Get a string literal by index
    const std::vector<std::string>& getStringLiterals() const; // New: Get all string literals
};

#endif // COMPILER_H
