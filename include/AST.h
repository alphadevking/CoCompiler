#ifndef AST_H
#define AST_H

#include "Tokens.h"

// --- Abstract Syntax Tree Node ---
class ASTNode {
public:
    enum class Type {
        INTEGER,
        FLOAT,
        UNKNOWN,
        IDENTIFIER_EXPRESSION, // New: For variable references
        ASSIGNMENT_EXPRESSION, // New: For assignments like x = 10
        VARIABLE_DECLARATION,  // New: For declarations like var x = 10;
        IF_STATEMENT,          // New: For if statements
        BLOCK_STATEMENT,       // New: For a block of statements enclosed in {}
        PRINT_STATEMENT,       // New: For print statements
        STRING_LITERAL,        // New: For string literals
        BOOLEAN_LITERAL,       // New: For boolean literals (true/false)
        UNARY_EXPRESSION       // New: For unary expressions like !true, -5
    };

    virtual ~ASTNode() = default;
    virtual std::string toString() const = 0;
    virtual Type getType() const = 0;
};

// --- Expression Nodes ---
class Expression : public ASTNode {
public:
    virtual ~Expression() = default;
    virtual Type getType() const override = 0;
};

// --- Unary Expression Node ---
class UnaryExpression : public Expression {
private:
    Token op;
    Expression* right;

public:
    UnaryExpression(Token op, Expression* right) : op(op), right(right) {}

    ~UnaryExpression() {
        delete right;
    }

    Token getOp() const { return op; }
    Expression* getRight() const { return right; }

    std::string toString() const override {
        return "UnaryExpression(" + op.value + right->toString() + ")";
    }
    Type getType() const override {
        // Type of unary expression is the type of its operand (e.g., !boolean -> boolean)
        return right->getType();
    }
};

// --- Literal Node ---
class Literal : public Expression {
private:
    Token token;

public:
    Literal(Token token) : token(token) {}
    Token getToken() const { return token; }
    std::string toString() const override { return "Literal(" + token.value + ")"; }
    Type getType() const override {
        // Determine type based on the token type
        if (token.type == TokenType::INT_LITERAL) return Type::INTEGER;
        if (token.type == TokenType::FLOAT_LITERAL) return Type::FLOAT;
        if (token.type == TokenType::STRING_LITERAL) return Type::STRING_LITERAL; // Correctly identify string literals
        return Type::UNKNOWN; // Should not happen for literals
    }
};

// --- Boolean Literal Node ---
class BooleanLiteral : public Expression {
private:
    Token token;

public:
    BooleanLiteral(Token token) : token(token) {}
    Token getToken() const { return token; }
    std::string toString() const override { return "BooleanLiteral(" + token.value + ")"; }
    Type getType() const override { return Type::BOOLEAN_LITERAL; }
};

// --- Identifier Expression Node ---
class IdentifierExpression : public Expression {
private:
    Token identifier; // The token for the identifier name

public:
    IdentifierExpression(Token identifier) : identifier(identifier) {}
    Token getIdentifier() const { return identifier; }
    std::string toString() const override { return "Identifier(" + identifier.value + ")"; }
    // Type will be determined during semantic analysis (lookup in symbol table)
    Type getType() const override { return Type::IDENTIFIER_EXPRESSION; }
};

// --- Assignment Expression Node ---
class AssignmentExpression : public Expression {
private:
    Token identifier; // The token for the variable being assigned to
    Expression* value; // The expression whose value is being assigned

public:
    AssignmentExpression(Token identifier, Expression* value)
        : identifier(identifier), value(value) {}

    ~AssignmentExpression() {
        delete value;
    }

    Token getIdentifier() const { return identifier; }
    Expression* getValue() const { return value; }
    std::string toString() const override {
        return "Assignment(" + identifier.value + " = " + value->toString() + ")";
    }
    // Type is the type of the assigned value
    Type getType() const override { return value->getType(); }
};

// --- Binary Expression Node ---
class BinaryExpression : public Expression {
private:
    Expression* left;
    Token op;
    Expression* right;

public:
    BinaryExpression(Expression* left, Token op, Expression* right) : left(left), op(op), right(right) {}

    ~BinaryExpression() {
        delete left;
        delete right;
    }

    Expression* getLeft() const { return left; }
    Token getOp() const { return op; }
    Expression* getRight() const { return right; }
    std::string toString() const override {
        return "BinaryExpression(" + left->toString() + " " + op.value + " " + right->toString() + ")";
    }
    Type getType() const override {
        Type leftType = left->getType();
        Type rightType = right->getType();

        // Handle string concatenation for '+' operator
        if (op.type == TokenType::PLUS) {
            if (leftType == Type::STRING_LITERAL && rightType == Type::STRING_LITERAL) {
                return Type::STRING_LITERAL;
            }
            // If one is string and other is not, it's a type error for '+'
            if ((leftType == Type::STRING_LITERAL && (rightType == Type::INTEGER || rightType == Type::FLOAT)) ||
                ((leftType == Type::INTEGER || leftType == Type::FLOAT) && rightType == Type::STRING_LITERAL)) {
                return Type::UNKNOWN; // Indicate type error
            }
        }

        // For other operators or numeric '+'
        if (leftType == Type::UNKNOWN || rightType == Type::UNKNOWN) {
            return Type::UNKNOWN;
        }
        if (leftType == Type::FLOAT || rightType == Type::FLOAT) {
            return Type::FLOAT;
        } else if (leftType == Type::INTEGER || rightType == Type::INTEGER) {
            return Type::INTEGER;
        } else if (leftType == Type::BOOLEAN_LITERAL || rightType == Type::BOOLEAN_LITERAL) {
            return Type::BOOLEAN_LITERAL;
        }
        return Type::UNKNOWN; // Fallback for unhandled types
    }
};

// --- Variable Declaration Node ---
class VariableDeclaration : public ASTNode {
private:
    Token identifier; // The token for the variable name
    Expression* initializer; // Optional: the expression for the initial value

public:
    VariableDeclaration(Token identifier, Expression* initializer = nullptr)
        : identifier(identifier), initializer(initializer) {}

    ~VariableDeclaration() {
        delete initializer;
    }

    Token getIdentifier() const { return identifier; }
    Expression* getInitializer() const { return initializer; }
    std::string toString() const override {
        std::string s = "VarDecl(" + identifier.value;
        if (initializer) {
            s += " = " + initializer->toString();
        }
        s += ")";
        return s;
    }
    // Type is determined by the initializer or during semantic analysis
    Type getType() const override { return Type::VARIABLE_DECLARATION; }
};

// --- If Statement Node ---
class IfStatement : public ASTNode {
private:
    Expression* condition;
    ASTNode* thenBranch; // Can be a single statement or a block
    ASTNode* elseBranch; // Optional else branch

public:
    IfStatement(Expression* condition, ASTNode* thenBranch, ASTNode* elseBranch = nullptr)
        : condition(condition), thenBranch(thenBranch), elseBranch(elseBranch) {}

    ~IfStatement() {
        delete condition;
        delete thenBranch;
        if (elseBranch) {
            delete elseBranch;
        }
    }

    Expression* getCondition() const { return condition; }
    ASTNode* getThenBranch() const { return thenBranch; }
    ASTNode* getElseBranch() const { return elseBranch; }

    std::string toString() const override {
        std::string s = "IfStatement(Condition: " + condition->toString() + ", Then: " + thenBranch->toString();
        if (elseBranch) {
            s += ", Else: " + elseBranch->toString();
        }
        s += ")";
        return s;
    }
    Type getType() const override { return Type::IF_STATEMENT; }
};

// --- Block Statement Node ---
class BlockStatement : public ASTNode {
private:
    std::vector<ASTNode*> statements;

public:
    BlockStatement(const std::vector<ASTNode*>& statements) : statements(statements) {}

    ~BlockStatement() {
        for (ASTNode* stmt : statements) {
            delete stmt;
        }
    }

    const std::vector<ASTNode*>& getStatements() const { return statements; }

    std::string toString() const override {
        std::string s = "BlockStatement(\n";
        for (ASTNode* stmt : statements) {
            s += "  " + stmt->toString() + "\n";
        }
        s += ")";
        return s;
    }
    Type getType() const override { return Type::BLOCK_STATEMENT; }
};

// --- Print Statement Node ---
class PrintStatement : public ASTNode {
private:
    Expression* expression;

public:
    PrintStatement(Expression* expression) : expression(expression) {}

    ~PrintStatement() {
        delete expression;
    }

    Expression* getExpression() const { return expression; }

    std::string toString() const override {
        return "PrintStatement(" + expression->toString() + ")";
    }
    Type getType() const override { return Type::PRINT_STATEMENT; }
};

#endif // AST_H
