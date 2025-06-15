#include "Parser.h"
#include <iostream>

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), current_pos(0) {}

Token Parser::peek() {
    if (current_pos >= tokens.size()) {
        return Token(TokenType::EOF_TOKEN, "", 0, 0);
    }
    return tokens[current_pos];
}

Token Parser::advance() {
    Token token = peek();
    current_pos++;
    return token;
}

bool Parser::match(TokenType type) {
    if (peek().type == type) {
        advance();
        return true;
    }
    return false;
}

/**
 * @brief Consumes the current token if its type matches the expected type, otherwise reports an error.
 * @param type The expected TokenType.
 * @param message The error message to display if the type does not match.
 * @return The consumed Token.
 */
Token Parser::consume(TokenType type, const std::string& message) {
    if (peek().type == type) {
        return advance();
    }
    std::cerr << "Parser Error: " << message << " at L" << peek().line << ":C" << peek().column << std::endl;
    // In a real compiler, you'd implement error recovery here.
    // For now, return a dummy EOF token to prevent further errors.
    return Token(TokenType::EOF_TOKEN, "", peek().line, peek().column);
}

/**
 * @brief Parses an identifier and creates an IdentifierExpression AST node.
 * @return A pointer to an IdentifierExpression node, or nullptr if an error occurs.
 */
Expression* Parser::parseIdentifier() {
    Token identifier_token = consume(TokenType::IDENTIFIER, "Expected identifier");
    if (identifier_token.type == TokenType::EOF_TOKEN) return nullptr; // Error occurred
    return new IdentifierExpression(identifier_token);
}

/**
 * @brief Parses a primary expression (literals, parenthesized expressions, identifiers).
 * @return A pointer to an Expression node, or nullptr if an error occurs.
 */
Expression* Parser::primary() {
    if (match(TokenType::INT_LITERAL)) {
        return new Literal(tokens[current_pos - 1]);
    } else if (match(TokenType::FLOAT_LITERAL)) {
        // Create a generic Literal object for float literals
        return new Literal(tokens[current_pos - 1]);
    } else if (match(TokenType::STRING_LITERAL)) { // Handle string literals as generic Literals
        return new Literal(tokens[current_pos - 1]);
    } else if (match(TokenType::TRUE)) { // Handle boolean true
        return new BooleanLiteral(tokens[current_pos - 1]);
    } else if (match(TokenType::FALSE)) { // New: Handle boolean false
        return new BooleanLiteral(tokens[current_pos - 1]);
    } else if (match(TokenType::IDENTIFIER)) { // Handle identifiers
        return new IdentifierExpression(tokens[current_pos - 1]);
    } else if (match(TokenType::LPAREN)) {
        Expression* expr = expression();
        if (match(TokenType::RPAREN)) {
            return expr;
        } else {
            std::cerr << "Parser Error: Expected ')' after expression at L" << peek().line << ":C" << peek().column << std::endl;
            return nullptr;
        }
    }
    std::cerr << "Parser Error: Expected expression at L" << peek().line << ":C" << peek().column << std::endl;
    return nullptr;
}

/**
 * @brief Parses a unary expression (e.g., -expression).
 * For now, it just calls primary().
 * @return A pointer to an Expression node, or nullptr if an error occurs.
 */
/**
 * @brief Parses a unary expression (e.g., -expression, !expression).
 * @return A pointer to an Expression node, or nullptr if an error occurs.
 */
Expression* Parser::unary() {
    if (match(TokenType::BANG) || match(TokenType::MINUS)) {
        Token op = tokens[current_pos - 1];
        Expression* right = unary(); // Unary operators are right-associative
        if (!right) return nullptr; // Propagate error
        return new UnaryExpression(op, right);
    }
    return primary();
}

/**
 * @brief Parses a factor expression (multiplication and division).
 * @return A pointer to an Expression node, or nullptr if an error occurs.
 */
Expression* Parser::factor() {
    Expression* expr = unary(); // Changed from primary() to unary()

    while (match(TokenType::STAR) || match(TokenType::SLASH)) {
        Token op = tokens[current_pos - 1];
        Expression* right = unary(); // Changed from primary() to unary()
        if (!expr || !right) return nullptr; // Propagate error
        expr = new BinaryExpression(expr, op, right);
    }

    return expr;
}

/**
 * @brief Parses a term expression (addition and subtraction).
 * @return A pointer to an Expression node, or nullptr if an error occurs.
 */
Expression* Parser::term() {
    Expression* expr = factor();

    while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
        Token op = tokens[current_pos - 1];
        Expression* right = factor();
        if (!expr || !right) return nullptr; // Propagate error
        expr = new BinaryExpression(expr, op, right);
    }

    return expr;
}

/**
 * @brief Parses a comparison expression (>, <, >=, <=, ==, !=).
 * @return A pointer to an Expression node, or nullptr if an error occurs.
 */
Expression* Parser::comparison() {
    Expression* expr = term();

    while (match(TokenType::GREATER) || match(TokenType::LESS) ||
           match(TokenType::GREATER_EQUAL) || match(TokenType::LESS_EQUAL) ||
           match(TokenType::EQUAL_EQUAL) || match(TokenType::BANG_EQUAL)) {
        Token op = tokens[current_pos - 1];
        Expression* right = term();
        if (!expr || !right) return nullptr; // Propagate error
        expr = new BinaryExpression(expr, op, right);
    }

    return expr;
}

/**
 * @brief Parses a logical AND expression (&&).
 * @return A pointer to an Expression node, or nullptr if an error occurs.
 */
Expression* Parser::logical_and() {
    Expression* expr = comparison();

    while (match(TokenType::AND)) {
        Token op = tokens[current_pos - 1];
        Expression* right = comparison();
        if (!expr || !right) return nullptr; // Propagate error
        expr = new BinaryExpression(expr, op, right);
    }

    return expr;
}

/**
 * @brief Parses a logical OR expression (||).
 * @return A pointer to an Expression node, or nullptr if an error occurs.
 */
Expression* Parser::logical_or() {
    Expression* expr = logical_and();

    while (match(TokenType::OR)) {
        Token op = tokens[current_pos - 1];
        Expression* right = logical_and();
        if (!expr || !right) return nullptr; // Propagate error
        expr = new BinaryExpression(expr, op, right);
    }

    return expr;
}

/**
 * @brief Parses an assignment expression.
 * @return A pointer to an Expression node, or nullptr if an error occurs.
 */
Expression* Parser::assignment() {
    Expression* expr = logical_or(); // Start with logical_or (or higher precedence expression)

    if (match(TokenType::ASSIGN)) {
        Token equals = tokens[current_pos - 1];
        // The left-hand side must be an IdentifierExpression
        IdentifierExpression* identifier_expr = dynamic_cast<IdentifierExpression*>(expr);
        if (!identifier_expr) {
            std::cerr << "Parser Error: Invalid assignment target. Expected identifier at L" << equals.line << ":C" << equals.column << std::endl;
            return nullptr;
        }
        Expression* value = assignment(); // Assignment is right-associative
        if (!value) return nullptr; // Propagate error
        return new AssignmentExpression(identifier_expr->getIdentifier(), value);
    }

    return expr;
}

/**
 * @brief Parses a general expression, incorporating assignment.
 * @return A pointer to an Expression node, or nullptr if an error occurs.
 */
Expression* Parser::expression() {
    return assignment(); // Top-level expression now includes assignment
}

/**
 * @brief Parses a variable declaration statement.
 * Syntax: var identifier = expression;
 * @return A pointer to a VariableDeclaration node, or nullptr if an error occurs.
 */
ASTNode* Parser::parseVariableDeclaration() {
    consume(TokenType::VAR, "Expected 'var' keyword"); // Consume 'var'
    Token identifier = consume(TokenType::IDENTIFIER, "Expected identifier after 'var'");
    if (identifier.type == TokenType::EOF_TOKEN) return nullptr; // Error occurred

    Expression* initializer = nullptr;
    if (match(TokenType::ASSIGN)) {
        initializer = expression(); // Parse the initializer expression
        if (!initializer) return nullptr; // Propagate error
    }

    // Consume the semicolon after a variable declaration
    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration");

    return new VariableDeclaration(identifier, initializer);
}

/**
 * @brief Parses a block of statements enclosed in curly braces {}.
 * @return A pointer to a BlockStatement node, or nullptr if an error occurs.
 */
ASTNode* Parser::block() {
    consume(TokenType::LBRACE, "Expected '{' to start a block");
    std::vector<ASTNode*> statements;
    while (peek().type != TokenType::RBRACE && peek().type != TokenType::EOF_TOKEN) {
        ASTNode* statement = parseStatement();
        if (statement) {
            statements.push_back(statement);
        } else {
            // Error occurred in parsing a statement within the block
            // Implement error recovery or return nullptr
            return nullptr;
        }
    }
    consume(TokenType::RBRACE, "Expected '}' to end a block");
    return new BlockStatement(statements);
}

/**
 * @brief Parses an if statement.
 * Syntax: if (condition) { then_branch } else { else_branch }
 * @return A pointer to an IfStatement node, or nullptr if an error occurs.
 */
ASTNode* Parser::parseIfStatement() {
    consume(TokenType::IF, "Expected 'if' keyword");
    consume(TokenType::LPAREN, "Expected '(' after 'if'");
    Expression* condition = expression();
    if (!condition) return nullptr; // Propagate error
    consume(TokenType::RPAREN, "Expected ')' after if condition");

    ASTNode* thenBranch = block(); // The 'then' branch is a block
    if (!thenBranch) return nullptr; // Propagate error

    ASTNode* elseBranch = nullptr;
    // Add support for 'else' keyword and 'else if'
    if (match(TokenType::ELSE)) {
        if (peek().type == TokenType::IF) {
            elseBranch = parseIfStatement(); // 'else if' is just an 'else' followed by another 'if' statement
        } else {
            elseBranch = block();
        }
        if (!elseBranch) return nullptr;
    }

    return new IfStatement(condition, thenBranch, elseBranch);
}

/**
 * @brief Parses a print statement.
 * Syntax: print expression;
 * @return A pointer to a PrintStatement node, or nullptr if an error occurs.
 */
ASTNode* Parser::parsePrintStatement() {
    consume(TokenType::PRINT, "Expected 'print' keyword");
    consume(TokenType::LPAREN, "Expected '(' after 'print'");
    Expression* expr = expression();
    if (!expr) return nullptr; // Propagate error
    consume(TokenType::RPAREN, "Expected ')' after print expression");
    consume(TokenType::SEMICOLON, "Expected ';' after print statement");
    return new PrintStatement(expr);
}

/**
 * @brief Parses a single statement (either a variable declaration or an expression).
 * @return A pointer to an ASTNode (VariableDeclaration or Expression), or nullptr if an error occurs.
 */
ASTNode* Parser::parseStatement() {
    if (peek().type == TokenType::VAR) {
        return parseVariableDeclaration();
    } else if (peek().type == TokenType::IF) { // New: Handle if statements
        return parseIfStatement();
    } else if (peek().type == TokenType::PRINT) { // New: Handle print statements
        return parsePrintStatement();
    }
    // If none of the above, it must be an expression statement
    Expression* exprStmt = expression();
    if (!exprStmt) return nullptr; // Error occurred
    consume(TokenType::SEMICOLON, "Expected ';' after expression statement");
    return exprStmt;
}

/**
 * @brief Parses the entire token stream into an AST.
 * @return A pointer to the root ASTNode, or nullptr if an error occurs.
 */
/**
 * @brief Parses the entire token stream into an AST, handling multiple statements.
 * @return A pointer to the root ASTNode (a BlockStatement if multiple, or a single statement), or nullptr if an error occurs.
 */
ASTNode* Parser::parse() {
    std::vector<ASTNode*> statements;
    while (peek().type != TokenType::EOF_TOKEN) {
        ASTNode* statement = parseStatement();
        if (statement) {
            statements.push_back(statement);
        } else {
            // Error occurred in parsing a statement, attempt to recover or stop
            // For now, we'll stop parsing on the first error in the top-level.
            return nullptr;
        }
    }

    if (statements.empty()) {
        return nullptr; // Empty input
    } else if (statements.size() == 1) {
        return statements[0]; // If only one statement, return it directly
    } else {
        return new BlockStatement(statements); // If multiple, wrap in a BlockStatement
    }
}
