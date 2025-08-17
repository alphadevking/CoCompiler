#include "../include/Compiler.h"
#include <iostream>
#include "../include/Tokens.h"
#include "../include/Bytecode.h"
#include "../include/AST.h"
#include "../include/SymbolTable.h" // Include for SymbolTable

/**
 * @brief Constructs a new Compiler object.
 * Initializes the symbol table with a global scope.
 */
Compiler::Compiler() : symbolTable() {
    // The symbolTable is initialized in the member initializer list,
    // which automatically calls its constructor and enters the global scope.
}

/**
 * @brief Compiles an Abstract Syntax Tree (AST) into a vector of bytecode instructions.
 * Manages the global scope for symbol table.
 *
 * @param ast A pointer to the root node of the AST.
 * @return A vector of Bytecode instructions representing the compiled code.
 */
std::vector<Bytecode> Compiler::compile(ASTNode* ast) {
    bytecode.clear();
    clearSemanticErrors();

    // Enter the global scope for compilation
    symbolTable.enterScope();

    compileNode(ast);

    // Exit the global scope after compilation
    symbolTable.exitScope();

    if (ast == nullptr || bytecode.empty()) {
        return {}; // Return empty vector if compilation failed or no bytecode was generated
    }
    bytecode.push_back(Bytecode(Instruction::HALT));

    // After all phases, report semantic errors and prevent execution if any
    if (!semanticErrors.empty()) {
        std::cerr << "--- Semantic Errors ---" << std::endl;
        for (const auto& err : semanticErrors) {
            std::cerr << err << std::endl;
        }
        std::cerr << "-----------------------" << std::endl;
        return {}; // Do NOT emit code if there are semantic errors
    }
    return bytecode;
}

/**
 * @brief Helper function to determine the type of a literal.
 *
 * @param literal A pointer to the Literal node.
 * @return The type of the literal.
 */
ASTNode::Type Compiler::getLiteralType(Literal* literal) {
    Token token = literal->getToken();
    if (token.type == TokenType::INT_LITERAL) {
        return ASTNode::Type::INTEGER;
    } else if (token.type == TokenType::FLOAT_LITERAL) {
        return ASTNode::Type::FLOAT;
    } else if (token.type == TokenType::STRING_LITERAL) {
        return ASTNode::Type::STRING_LITERAL;
    } else if (token.type == TokenType::TRUE || token.type == TokenType::FALSE) {
        return ASTNode::Type::BOOLEAN_LITERAL;
    }
    return ASTNode::Type::UNKNOWN;
}

/**
 * @brief Compiles a single node of the AST.
 * Performs semantic analysis (type checking, symbol lookup) and bytecode generation.
 *
 * @param node A pointer to the AST node to compile.
 */
void Compiler::compileNode(ASTNode* node) {
    if (!node) {
        return; // Handle null nodes gracefully
    }

    // Compile Literal node
    if (Literal* literal = dynamic_cast<Literal*>(node)) {
        Token token = literal->getToken();
        if (token.type == TokenType::INT_LITERAL) {
            int value = std::stoi(token.value);
            bytecode.push_back(Bytecode(Instruction::PUSH_INT, value));
        } else if (token.type == TokenType::FLOAT_LITERAL) {
            float value = std::stof(token.value);
            // Pass float value directly to Bytecode constructor, which now handles double operand
            bytecode.push_back(Bytecode(Instruction::PUSH_FLOAT, value));
        } else if (token.type == TokenType::STRING_LITERAL) {
            // Store the string literal and push its index
            int index = static_cast<int>(string_literals.size());
            string_literals.push_back(token.value);
            bytecode.push_back(Bytecode(Instruction::PUSH_STRING, index));
        }
    }
    // Compile BooleanLiteral node
    else if (BooleanLiteral* boolLiteral = dynamic_cast<BooleanLiteral*>(node)) {
        /**
         * @brief Compiles a BooleanLiteral node.
         * Pushes 1 for true, 0 for false onto the bytecode stack.
         * @param boolLiteral The BooleanLiteral AST node.
         */
        if (boolLiteral->getToken().type == TokenType::TRUE) {
            bytecode.push_back(Bytecode(Instruction::PUSH_INT, 1)); // true as 1
        } else {
            bytecode.push_back(Bytecode(Instruction::PUSH_INT, 0)); // false as 0
        }
    }
    // Compile IdentifierExpression node (variable reference)
    else if (IdentifierExpression* identExpr = dynamic_cast<IdentifierExpression*>(node)) {
        Token identifier_token = identExpr->getIdentifier();
        Symbol* symbol = symbolTable.lookupSymbol(identifier_token.value);
        if (!symbol) {
            semanticErrors.push_back("Semantic Error: Undeclared variable '" + identifier_token.value + "' at L" + std::to_string(identifier_token.line) + ":C" + std::to_string(identifier_token.column));
            return;
        }
        // Push the address of the variable onto the stack, then load its value
        bytecode.push_back(Bytecode(Instruction::PUSH_INT, symbol->address));
        bytecode.push_back(Bytecode(Instruction::LOAD));
    }
    // Compile AssignmentExpression node (variable assignment)
    else if (AssignmentExpression* assignExpr = dynamic_cast<AssignmentExpression*>(node)) {
        Token identifier_token = assignExpr->getIdentifier();
        Symbol* symbol = symbolTable.lookupSymbol(identifier_token.value);
        if (!symbol) {
            semanticErrors.push_back("Semantic Error: Assignment to undeclared variable '" + identifier_token.value + "' at L" + std::to_string(identifier_token.line) + ":C" + std::to_string(identifier_token.column));
            return;
        }

        // Compile the right-hand side expression
        compileNode(assignExpr->getValue());
        if (bytecode.empty()) return; // Propagate error

        // Type checking for assignment (basic)
        // Resolve the actual type of the right-hand side expression
        ASTNode::Type assignedType = resolveExpressionType(assignExpr->getValue());
        if (symbol->type != ASTNode::Type::UNKNOWN && assignedType != ASTNode::Type::UNKNOWN && symbol->type != assignedType) {
            semanticErrors.push_back("Semantic Error: Type mismatch in assignment for variable '" + identifier_token.value + "'. Expected " +
                (symbol->type == ASTNode::Type::INTEGER ? "INTEGER" :
                (symbol->type == ASTNode::Type::FLOAT ? "FLOAT" :
                (symbol->type == ASTNode::Type::STRING_LITERAL ? "STRING" : "BOOLEAN"))) +
                ", got " +
                (assignedType == ASTNode::Type::INTEGER ? "INTEGER" :
                (assignedType == ASTNode::Type::FLOAT ? "FLOAT" :
                (assignedType == ASTNode::Type::STRING_LITERAL ? "STRING" : "BOOLEAN"))) +
                " at L" + std::to_string(identifier_token.line) + ":C" + std::to_string(identifier_token.column));
            return;
        }
        // If symbol type was UNKNOWN (e.g., from declaration without initializer), set it now
        if (symbol->type == ASTNode::Type::UNKNOWN) {
            symbol->type = assignedType;
        }

        // Push the address of the variable onto the stack, then store the value
        bytecode.push_back(Bytecode(Instruction::PUSH_INT, symbol->address));
        bytecode.push_back(Bytecode(Instruction::STORE));
    }
    // Compile BinaryExpression node
    else if (BinaryExpression* binaryExpr = dynamic_cast<BinaryExpression*>(node)) {
        ASTNode* left = binaryExpr->getLeft();
        ASTNode* right = binaryExpr->getRight();
        Token op = binaryExpr->getOp();

        // Resolve the actual types of the left and right operands
        ASTNode::Type leftType;
        if (Literal* lit = dynamic_cast<Literal*>(left)) {
            leftType = getLiteralType(lit);
        } else {
            leftType = resolveExpressionType(dynamic_cast<Expression*>(left));
        }

        ASTNode::Type rightType;
        if (Literal* lit = dynamic_cast<Literal*>(right)) {
            rightType = getLiteralType(lit);
        } else {
            rightType = resolveExpressionType(dynamic_cast<Expression*>(right));
        }

        // Handle logical operators (AND, OR)
        if (op.type == TokenType::AND) {
            if (!((leftType == ASTNode::Type::BOOLEAN_LITERAL || leftType == ASTNode::Type::INTEGER) &&
                  (rightType == ASTNode::Type::BOOLEAN_LITERAL || rightType == ASTNode::Type::INTEGER))) {
                semanticErrors.push_back("Semantic Error: Logical operator '&&' requires boolean or integer operands.");
                return;
            }
            // Short-circuiting AND logic
            compileNode(left); // Evaluate left operand (leaves 0 or 1 on stack)
            if (bytecode.empty()) return;

            // If left is false (0), jump to L_FALSE.
            // The value of 'left' is on the stack.
            int jumpToFalse_idx = static_cast<int>(bytecode.size());
            bytecode.push_back(Bytecode(Instruction::JUMP_IF_FALSE, 0)); // Placeholder for L_FALSE

            // If left was true, the stack is now empty (JUMP_IF_FALSE popped it).
            // Compile right operand. Its result is the AND result.
            compileNode(right); // Evaluate right operand (leaves 0 or 1 on stack)
            if (bytecode.empty()) return;

            // After compiling right, its result is on the stack. This is the final result if left was true.
            // Jump over the 'false' path.
            int jumpToEnd_idx = static_cast<int>(bytecode.size());
            bytecode.push_back(Bytecode(Instruction::JUMP, 0)); // Placeholder for L_END

            // L_FALSE: (This is where we jump if the left operand was false)
            bytecode[jumpToFalse_idx].operand = static_cast<int>(bytecode.size());
            // The stack is already empty (JUMP_IF_FALSE popped it).
            bytecode.push_back(Bytecode(Instruction::PUSH_INT, 0)); // Push 0 (false)

            // L_END: (This is where execution continues after the AND expression)
            bytecode[jumpToEnd_idx].operand = static_cast<int>(bytecode.size());

        } else if (op.type == TokenType::OR) {
            if (!((leftType == ASTNode::Type::BOOLEAN_LITERAL || leftType == ASTNode::Type::INTEGER) &&
                  (rightType == ASTNode::Type::BOOLEAN_LITERAL || rightType == ASTNode::Type::INTEGER))) {
                semanticErrors.push_back("Semantic Error: Logical operator '||' requires boolean or integer operands.");
                return;
            }
            // Short-circuiting OR logic
            compileNode(left); // Evaluate left operand (leaves 0 or 1 on stack)
            if (bytecode.empty()) return;

            // If left is true (1), jump to L_TRUE.
            // JUMP_IF_TRUE will pop the value of 'left'.
            int jumpToTrue_idx = static_cast<int>(bytecode.size());
            bytecode.push_back(Bytecode(Instruction::JUMP_IF_TRUE, 0)); // Placeholder for L_TRUE

            // If left was false, the stack is now empty (JUMP_IF_TRUE popped it).
            // Compile right operand. Its result is the OR result.
            compileNode(right); // Evaluate right operand (leaves 0 or 1 on stack)
            if (bytecode.empty()) return;

            // After compiling right, its result is on the stack. This is the final result if left was false.
            // Jump over the 'true' path.
            int jumpToEnd_idx = static_cast<int>(bytecode.size());
            bytecode.push_back(Bytecode(Instruction::JUMP, 0)); // Placeholder for L_END

            // L_TRUE: (This is where we jump if the left operand was true)
            bytecode[jumpToTrue_idx].operand = static_cast<int>(bytecode.size());
            // The stack is already empty (JUMP_IF_TRUE popped it).
            bytecode.push_back(Bytecode(Instruction::PUSH_INT, 1)); // Push 1 (true)

            // L_END: (This is where execution continues after the OR expression)
            bytecode[jumpToEnd_idx].operand = static_cast<int>(bytecode.size());

        } else if (op.type == TokenType::PLUS) {
            // --- NEW IMPLEMENTATION (v2) ---
            // Handle string concatenation using resolved types
            if (leftType == ASTNode::Type::STRING_LITERAL && rightType == ASTNode::Type::STRING_LITERAL) {
                compileNode(left);
                if (bytecode.empty()) return;
                compileNode(right);
                if (bytecode.empty()) return;
                bytecode.push_back(Bytecode(Instruction::CONCAT_STRING));
            }
            // --- END NEW IMPLEMENTATION (v2) ---
            // Handle numeric addition
            else if ((leftType == ASTNode::Type::INTEGER || leftType == ASTNode::Type::FLOAT) &&
                     (rightType == ASTNode::Type::INTEGER || rightType == ASTNode::Type::FLOAT)) {
                compileNode(left);
                if (bytecode.empty()) return;
                compileNode(right);
                if (bytecode.empty()) return;
                bytecode.push_back(Bytecode(Instruction::ADD));
            } else {
                semanticErrors.push_back("Semantic Error: Operator '+' requires two numeric operands or two string operands for concatenation.");
                return;
            }
        } else if (op.type == TokenType::MINUS || op.type == TokenType::STAR || op.type == TokenType::SLASH) {
            // Handle numeric arithmetic operators
            if (!((leftType == ASTNode::Type::INTEGER || leftType == ASTNode::Type::FLOAT) &&
                  (rightType == ASTNode::Type::INTEGER || rightType == ASTNode::Type::FLOAT))) {
                semanticErrors.push_back("Semantic Error: Arithmetic operator '" + op.value + "' requires numeric operands.");
                return;
            }
            compileNode(left);
            if (bytecode.empty()) return;
            compileNode(right);
            if (bytecode.empty()) return;

            if (op.type == TokenType::MINUS) {
                bytecode.push_back(Bytecode(Instruction::SUB));
            } else if (op.type == TokenType::STAR) {
                bytecode.push_back(Bytecode(Instruction::MUL));
            } else if (op.type == TokenType::SLASH) {
                bytecode.push_back(Bytecode(Instruction::DIV));
            }
        } else if (op.type == TokenType::GREATER || op.type == TokenType::LESS ||
                   op.type == TokenType::GREATER_EQUAL || op.type == TokenType::LESS_EQUAL ||
                   op.type == TokenType::EQUAL_EQUAL || op.type == TokenType::BANG_EQUAL) {
            // Handle comparison operators
            if (!((leftType == ASTNode::Type::INTEGER || leftType == ASTNode::Type::FLOAT) &&
                  (rightType == ASTNode::Type::INTEGER || rightType == ASTNode::Type::FLOAT))) {
                semanticErrors.push_back("Semantic Error: Comparison operator '" + op.value + "' requires numeric operands.");
                return;
            }
            compileNode(left);
            if (bytecode.empty()) return;
            compileNode(right);
            if (bytecode.empty()) return;

            if (op.type == TokenType::GREATER) {
                bytecode.push_back(Bytecode(Instruction::GREATER));
            } else if (op.type == TokenType::LESS) {
                bytecode.push_back(Bytecode(Instruction::LESS));
            } else if (op.type == TokenType::GREATER_EQUAL) {
                bytecode.push_back(Bytecode(Instruction::GREATER_EQUAL));
            } else if (op.type == TokenType::LESS_EQUAL) {
                bytecode.push_back(Bytecode(Instruction::LESS_EQUAL));
            } else if (op.type == TokenType::EQUAL_EQUAL) {
                bytecode.push_back(Bytecode(Instruction::EQUAL_EQUAL));
            } else if (op.type == TokenType::BANG_EQUAL) {
                bytecode.push_back(Bytecode(Instruction::BANG_EQUAL));
            }
        } else {
            semanticErrors.push_back("Semantic Error: Unknown binary operator '" + op.value + "'.");
            return;
        }
    }
    // Compile UnaryExpression node
    else if (UnaryExpression* unaryExpr = dynamic_cast<UnaryExpression*>(node)) {
        compileNode(unaryExpr->getRight());
        if (bytecode.empty()) return; // Propagate error

        Token op = unaryExpr->getOp();
        if (op.type == TokenType::BANG) {
            bytecode.push_back(Bytecode(Instruction::NOT));
        } else if (op.type == TokenType::MINUS) {
            bytecode.push_back(Bytecode(Instruction::NEGATE)); // Emit NEGATE instruction
        } else {
            semanticErrors.push_back("Semantic Error: Unknown unary operator.");
            return;
        }
    }
    // Compile VariableDeclaration node
    else if (VariableDeclaration* varDecl = dynamic_cast<VariableDeclaration*>(node)) {
        Token identifier_token = varDecl->getIdentifier();
        Expression* initializer = varDecl->getInitializer();

        // Determine the type of the variable
        ASTNode::Type varType = ASTNode::Type::UNKNOWN;
        if (initializer) {
            varType = initializer->getType();
            if (varType == ASTNode::Type::IDENTIFIER_EXPRESSION) {
                // If initializer is an identifier, look up its type
                Symbol* initSymbol = symbolTable.lookupSymbol(dynamic_cast<IdentifierExpression*>(initializer)->getIdentifier().value);
                if (initSymbol) {
                    varType = initSymbol->type;
                } else {
                    semanticErrors.push_back("Semantic Error: Initializer for variable '" + identifier_token.value + "' is an undeclared variable at L" + std::to_string(identifier_token.line) + ":C" + std::to_string(identifier_token.column));
                    return;
                }
            }
        } else {
            // If no initializer, the type is initially UNKNOWN and will be determined upon first assignment.
            varType = ASTNode::Type::UNKNOWN;
        }

        // Add the symbol to the symbol table
        if (!symbolTable.addSymbol(identifier_token.value, varType)) {
            // Error already reported by addSymbol if symbol exists
            semanticErrors.push_back("Semantic Error: Duplicate variable '" + identifier_token.value + "' at L" + std::to_string(identifier_token.line) + ":C" + std::to_string(identifier_token.column));
            return;
        }
        // If there's an initializer, compile it and store the result
        if (initializer) {
            compileNode(initializer);
            if (bytecode.empty()) return; // Propagate error

            // Get the address of the newly added symbol
            Symbol* symbol = symbolTable.lookupSymbol(identifier_token.value);
            if (!symbol) { // Should not happen if addSymbol was successful
                semanticErrors.push_back("Internal Compiler Error: Symbol not found after adding it.");
                return;
            }
            bytecode.push_back(Bytecode(Instruction::PUSH_INT, symbol->address));
            bytecode.push_back(Bytecode(Instruction::STORE));
        }
    }
    // Compile BlockStatement node
    else if (BlockStatement* blockStmt = dynamic_cast<BlockStatement*>(node)) {
        // Enter a new scope for the block
        symbolTable.enterScope();
        for (ASTNode* stmt : blockStmt->getStatements()) {
            compileNode(stmt);
            if (bytecode.empty()) return; // Propagate error
        }
        // Exit the scope after compiling the block
        symbolTable.exitScope();
    }
    // Compile IfStatement node
    else if (IfStatement* ifStmt = dynamic_cast<IfStatement*>(node)) {
        // Compile the condition
        compileNode(ifStmt->getCondition());
        if (bytecode.empty()) return; // Propagate error

        // JUMP_IF_FALSE instruction: if condition is false, jump to the end of the then-branch
        // The operand (jump target) will be patched later
        int jumpIfFalseAddress = static_cast<int>(bytecode.size());
        bytecode.push_back(Bytecode(Instruction::JUMP_IF_FALSE, 0)); // Placeholder address

        // Compile the then-branch
        compileNode(ifStmt->getThenBranch());
        if (bytecode.empty()) return; // Propagate error

        // If there's an else-branch, we need an unconditional jump to skip it
        int jumpToEndAddress = -1;
        if (ifStmt->getElseBranch()) {
            jumpToEndAddress = static_cast<int>(bytecode.size());
            bytecode.push_back(Bytecode(Instruction::JUMP, 0)); // Placeholder address
        }

        // Patch the JUMP_IF_FALSE instruction to point to the start of the else-branch or end of if
        bytecode[jumpIfFalseAddress].operand = static_cast<int>(bytecode.size());

        // Compile the else-branch if it exists
        if (ifStmt->getElseBranch()) {
            compileNode(ifStmt->getElseBranch());
            if (bytecode.empty()) return; // Propagate error
            // Patch the unconditional JUMP instruction to point to the end of the else-branch
            bytecode[jumpToEndAddress].operand = static_cast<int>(bytecode.size());
        }
    }
    // Compile PrintStatement node
    else if (PrintStatement* printStmt = dynamic_cast<PrintStatement*>(node)) {
        Expression* expr = printStmt->getExpression();
        compileNode(expr);
        if (bytecode.empty()) return; // Propagate error

        // Determine if we are printing a string, a boolean, or a value
        ASTNode::Type exprType = resolveExpressionType(expr);
        if (exprType == ASTNode::Type::STRING_LITERAL) {
            bytecode.push_back(Bytecode(Instruction::PRINT_STRING));
        } else if (exprType == ASTNode::Type::BOOLEAN_LITERAL) {
            bytecode.push_back(Bytecode(Instruction::PRINT_BOOL));
        } else {
            bytecode.push_back(Bytecode(Instruction::PRINT_VALUE));
        }
    }
    else {
        semanticErrors.push_back("Semantic Error: Unknown AST node type encountered.");
        return;
    }
}

/**
 * @brief Returns a string literal by its index.
 * @param index The index of the string literal.
 * @return The string literal.
 */
const std::string& Compiler::getStringLiteral(int index) const {
    if (index >= 0 && index < string_literals.size()) {
        return string_literals[index];
    }
    // Handle error: index out of bounds
    static const std::string error_str = "ERROR: String literal index out of bounds";
    std::cerr << error_str << std::endl;
    return error_str;
}

/**
 * @brief Resolves the actual type of an expression, especially for identifiers.
 *
 * @param expr A pointer to the Expression node.
 * @return The resolved type of the expression.
 */
ASTNode::Type Compiler::resolveExpressionType(Expression* expr) {
    if (IdentifierExpression* identExpr = dynamic_cast<IdentifierExpression*>(expr)) {
        Token identifier_token = identExpr->getIdentifier();
        Symbol* symbol = symbolTable.lookupSymbol(identifier_token.value);
        if (symbol) {
            return symbol->type;
        } else {
            // If symbol not found, it's an undeclared variable.
            // This error should ideally be caught earlier or handled more gracefully.
            // For now, return UNKNOWN and let subsequent checks handle it.
            semanticErrors.push_back("Semantic Warning: Attempted to resolve type of undeclared variable '" + identifier_token.value + "' at L" + std::to_string(identifier_token.line) + ":C" + std::to_string(identifier_token.column));
            return ASTNode::Type::UNKNOWN;
        }
    } else if (BinaryExpression* binExpr = dynamic_cast<BinaryExpression*>(expr)) {
        // For binary expressions, determine the resulting type based on the operator and operand types
        ASTNode::Type leftType = resolveExpressionType(binExpr->getLeft());
        ASTNode::Type rightType = resolveExpressionType(binExpr->getRight());
        TokenType opType = binExpr->getOp().type;

        if (opType == TokenType::PLUS && leftType == ASTNode::Type::STRING_LITERAL && rightType == ASTNode::Type::STRING_LITERAL) {
            return ASTNode::Type::STRING_LITERAL;
        }
        // For comparison and logical operators, the result is always a boolean.
        if (opType == TokenType::GREATER || opType == TokenType::LESS ||
            opType == TokenType::GREATER_EQUAL || opType == TokenType::LESS_EQUAL ||
            opType == TokenType::EQUAL_EQUAL || opType == TokenType::BANG_EQUAL ||
            opType == TokenType::AND || opType == TokenType::OR) {
            return ASTNode::Type::BOOLEAN_LITERAL;
        }

        // For arithmetic operators, determine the resulting numeric type.
        if ((leftType == ASTNode::Type::INTEGER || leftType == ASTNode::Type::FLOAT) &&
            (rightType == ASTNode::Type::INTEGER || rightType == ASTNode::Type::FLOAT)) {
            if (leftType == ASTNode::Type::FLOAT || rightType == ASTNode::Type::FLOAT) {
                return ASTNode::Type::FLOAT;
            } else {
                return ASTNode::Type::INTEGER;
            }
        }
        return ASTNode::Type::UNKNOWN; // Fallback for unhandled binary expression types or type mismatches
    } else if (UnaryExpression* unaryExpr = dynamic_cast<UnaryExpression*>(expr)) {
        // For unary NOT operator, the result is a boolean.
        if (unaryExpr->getOp().type == TokenType::BANG) {
            return ASTNode::Type::BOOLEAN_LITERAL;
        }
        // For unary MINUS, the result is numeric.
        return resolveExpressionType(unaryExpr->getRight());
    } else {
        // For other expression types, just return their inherent type
        return expr->getType();
    }
}

/**
 * @brief Returns all stored string literals.
 * @return A constant reference to the vector of string literals.
 */
const std::vector<std::string>& Compiler::getStringLiterals() const {
    return string_literals;
}

const std::vector<std::string>& Compiler::getSemanticErrors() const {
    return semanticErrors;
}

void Compiler::clearSemanticErrors() {
    semanticErrors.clear();
}
