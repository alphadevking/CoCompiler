// main.cpp
// This is the main file. It runs all the steps of our simple compiler, with clear comments for each phase.

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <regex>
#include <sstream> // Added for istringstream and getline
#include "Tokens.h"
#include "include/Lexer.h"
#include "include/Parser.h"
#include "include/AST.h"
#include "include/Compiler.h"
#include "include/VM.h"
#include <unordered_set>
#include <typeinfo> // Added for typeid

// Forward declarations for functions used in run_compiler
std::string preprocess(const std::string& source_code);
bool semantic_check(ASTNode* ast, std::unordered_set<std::string>& declared_vars);

// === Three-Address Code (TAC) Structures ===
struct TACInstruction {
    std::string op;      // Operation, e.g., "+", "=", "print"
    std::string dest;    // Destination variable or temp
    std::string src1;    // First source operand
    std::string src2;    // Second source operand (if any)

    TACInstruction(const std::string& op, const std::string& dest, const std::string& src1 = "", const std::string& src2 = "")
        : op(op), dest(dest), src1(src1), src2(src2) {}
};

// TAC generation helpers
typedef std::vector<TACInstruction> TACList;
int tac_temp_counter = 0;
TACList tac_instructions;

std::string new_temp() {
    return "t" + std::to_string(++tac_temp_counter);
}

std::string generate_tac(ASTNode* node) {
    if (!node) return "";
    if (auto* lit = dynamic_cast<Literal*>(node)) {
        std::string temp = new_temp();
        tac_instructions.emplace_back("=", temp, lit->getToken().value);
        return temp;
    }
    if (auto* ident = dynamic_cast<IdentifierExpression*>(node)) {
        return ident->getIdentifier().value;
    }
    if (auto* bin = dynamic_cast<BinaryExpression*>(node)) {
        std::string left = generate_tac(bin->getLeft());
        std::string right = generate_tac(bin->getRight());
        std::string temp = new_temp();
        tac_instructions.emplace_back(bin->getOp().value, temp, left, right);
        return temp;
    }
    if (auto* assign = dynamic_cast<AssignmentExpression*>(node)) {
        std::string value = generate_tac(assign->getValue());
        std::string var = assign->getIdentifier().value;
        tac_instructions.emplace_back("=", var, value);
        return var;
    }
    if (auto* varDecl = dynamic_cast<VariableDeclaration*>(node)) {
        std::string value = varDecl->getInitializer() ? generate_tac(varDecl->getInitializer()) : "0";
        std::string var = varDecl->getIdentifier().value;
        tac_instructions.emplace_back("=", var, value);
        return var;
    }
    if (auto* printstmt = dynamic_cast<PrintStatement*>(node)) {
        std::string value = generate_tac(printstmt->getExpression());
        tac_instructions.emplace_back("print", "", value);
        return "";
    }
    if (auto* block = dynamic_cast<BlockStatement*>(node)) {
        for (auto* stmt : block->getStatements()) {
            generate_tac(stmt);
        }
        return "";
    }
    if (auto* ifstmt = dynamic_cast<IfStatement*>(node)) {
        std::string cond = generate_tac(ifstmt->getCondition());
        std::string label_else = "L" + std::to_string(++tac_temp_counter);
        std::string label_end = "L" + std::to_string(++tac_temp_counter);
        tac_instructions.emplace_back("ifz_goto", "", cond, label_else);
        generate_tac(ifstmt->getThenBranch());
        tac_instructions.emplace_back("goto", "", label_end);
        tac_instructions.emplace_back("label", label_else);
        if (ifstmt->getElseBranch()) {
            generate_tac(ifstmt->getElseBranch());
        }
        tac_instructions.emplace_back("label", label_end);
        return "";
    }
    return "";
}

void print_tokens(const std::vector<Token>& tokens) {
    std::cout << "\n=== Token Stream ===" << std::endl;
    for (const auto& token : tokens) {
        std::cout << token.toString() << std::endl;
    }
    std::cout << "--------------------" << std::endl;
}

void print_tac() {
    std::cout << "\n=== Three-Address Code (TAC) ===" << std::endl;
    for (const auto& instr : tac_instructions) {
        if (instr.op == "label") {
            std::cout << instr.dest << ":" << std::endl;
        } else if (instr.op == "goto") {
            std::cout << "    goto " << instr.src1 << std::endl;
        } else if (instr.op == "ifz_goto") {
            std::cout << "    ifz " << instr.src1 << " goto " << instr.src2 << std::endl;
        } else if (instr.op == "print") {
            std::cout << "    print " << instr.src1 << std::endl;
        } else if (instr.op == "=" && instr.src2.empty()) {
            std::cout << "    " << instr.dest << " = " << instr.src1 << std::endl;
        } else {
            std::cout << "    " << instr.dest << " = " << instr.src1 << " " << instr.op << " " << instr.src2 << std::endl;
        }
    }
}

// === Control Flow Graph (CFG) Structures ===
#include <map>
#include <set>

struct CFGNode {
    std::string label; // Block label (e.g., L1)
    std::vector<std::string> instructions; // TAC instructions in this block
    std::set<std::string> edges; // Outgoing edges (labels)
};

std::map<std::string, CFGNode> build_cfg(const TACList& tac) {
    std::map<std::string, CFGNode> cfg;
    std::string current_label = "entry";
    cfg[current_label] = CFGNode{current_label, {}, {}};
    for (size_t i = 0; i < tac.size(); ++i) {
        const auto& instr = tac[i];
        if (instr.op == "label") {
            current_label = instr.dest;
            if (!cfg.count(current_label))
                cfg[current_label] = CFGNode{current_label, {}, {}};
            continue;
        }
        std::string instr_str;
        if (instr.op == "goto") {
            instr_str = "goto " + instr.src1;
            cfg[current_label].instructions.push_back(instr_str);
            cfg[current_label].edges.insert(instr.src1);
            // Start a new block after goto
            if (i + 1 < tac.size() && tac[i + 1].op == "label") {
                current_label = tac[i + 1].dest;
            } else {
                current_label = "block_after_goto_" + std::to_string(i);
                cfg[current_label] = CFGNode{current_label, {}, {}};
            }
            continue;
        } else if (instr.op == "ifz_goto") {
            instr_str = "ifz " + instr.src1 + " goto " + instr.src2;
            cfg[current_label].instructions.push_back(instr_str);
            cfg[current_label].edges.insert(instr.src2);
            // Also add a fallthrough edge to the next block
            std::string fallthrough = (i + 1 < tac.size() && tac[i + 1].op == "label") ? tac[i + 1].dest : ("block_fallthrough_" + std::to_string(i));
            cfg[current_label].edges.insert(fallthrough);
            continue;
        } else if (instr.op == "print") {
            instr_str = "print " + instr.src1;
        } else if (instr.op == "=" && instr.src2.empty()) {
            instr_str = instr.dest + " = " + instr.src1;
        } else if (instr.op == "=" && !instr.src2.empty()) {
            instr_str = instr.dest + " = " + instr.src1;
        } else if (instr.op != "label") {
            instr_str = instr.dest + " = " + instr.src1 + " " + instr.op + " " + instr.src2;
        }
        if (!instr_str.empty())
            cfg[current_label].instructions.push_back(instr_str);
    }
    return cfg;
}

void print_cfg(const std::map<std::string, CFGNode>& cfg) {
    std::cout << "\n=== Control Flow Graph (CFG) ===" << std::endl;
    for (const auto& [label, node] : cfg) {
        std::cout << "Block: " << label << std::endl;
        for (const auto& instr : node.instructions) {
            std::cout << "    " << instr << std::endl;
        }
        std::cout << "    Edges: ";
        for (const auto& edge : node.edges) {
            std::cout << edge << " ";
        }
        std::cout << std::endl;
    }
}

// === Simple SSA Transformation for If-Else with Phi Functions ===
#include <unordered_map>
#include <set>

TACList ssa_tac;

// Helper to find variables assigned in a TAC block
std::set<std::string> assigned_vars(const TACList& block) {
    std::set<std::string> vars;
    for (const auto& instr : block) {
        if (!instr.dest.empty() && instr.op != "label" && instr.op != "print") {
            vars.insert(instr.dest);
        }
    }
    return vars;
}

// Minimal SSA with phi for top-level if-else only
void to_ssa_with_phi(const TACList& tac) {
    ssa_tac.clear();
    std::unordered_map<std::string, int> version;
    std::unordered_map<std::string, std::string> current_name;
    size_t i = 0;
    while (i < tac.size()) {
        const auto& instr = tac[i];
        if (instr.op == "ifz_goto") {
            // Find then and else blocks
            size_t then_start = i + 1;
            size_t else_label_idx = 0, end_label_idx = 0;
            std::string else_label = instr.src2;
            // Find else label
            for (size_t j = then_start; j < tac.size(); ++j) {
                if (tac[j].op == "label" && tac[j].dest == else_label) {
                    else_label_idx = j;
                    break;
                }
            }
            // Find end label (after else)
            for (size_t j = else_label_idx + 1; j < tac.size(); ++j) {
                if (tac[j].op == "label") {
                    end_label_idx = j;
                    break;
                }
            }
            // SSA for then block
            TACList then_block;
            for (size_t j = then_start; j < else_label_idx; ++j) then_block.push_back(tac[j]);
            std::unordered_map<std::string, std::string> then_names = current_name;
            std::unordered_map<std::string, int> then_version = version;
            TACList then_ssa;
            for (const auto& t : then_block) {
                std::string dest = t.dest, src1 = t.src1, src2 = t.src2;
                if (!src1.empty() && then_names.count(src1)) src1 = then_names[src1];
                if (!src2.empty() && then_names.count(src2)) src2 = then_names[src2];
                if (!dest.empty()) {
                    then_version[dest]++;
                    then_names[dest] = dest + std::to_string(then_version[dest]);
                    dest = then_names[dest];
                }
                then_ssa.emplace_back(t.op, dest, src1, src2);
            }
            // SSA for else block
            TACList else_block;
            for (size_t j = else_label_idx + 1; j < end_label_idx; ++j) else_block.push_back(tac[j]);
            std::unordered_map<std::string, std::string> else_names = current_name;
            std::unordered_map<std::string, int> else_version = version;
            TACList else_ssa;
            for (const auto& t : else_block) {
                std::string dest = t.dest, src1 = t.src1, src2 = t.src2;
                if (!src1.empty() && else_names.count(src1)) src1 = else_names[src1];
                if (!src2.empty() && else_names.count(src2)) src2 = else_names[src2];
                if (!dest.empty()) {
                    else_version[dest]++;
                    else_names[dest] = dest + std::to_string(else_version[dest]);
                    dest = else_names[dest];
                }
                else_ssa.emplace_back(t.op, dest, src1, src2);
            }
            // Insert phi for variables assigned in both branches
            std::set<std::string> then_assigned, else_assigned;
            for (const auto& t : then_block) if (!t.dest.empty()) then_assigned.insert(t.dest);
            for (const auto& t : else_block) if (!t.dest.empty()) else_assigned.insert(t.dest);
            std::set<std::string> phi_vars;
            for (const auto& v : then_assigned) if (else_assigned.count(v)) phi_vars.insert(v);
            // Output: ifz_goto, then_ssa, else_ssa, phi
            ssa_tac.push_back(instr); // ifz_goto
            ssa_tac.insert(ssa_tac.end(), then_ssa.begin(), then_ssa.end());
            ssa_tac.insert(ssa_tac.end(), else_ssa.begin(), else_ssa.end());
            for (const auto& v : phi_vars) {
                std::string then_v = then_names[v];
                std::string else_v = else_names[v];
                version[v]++;
                std::string phi_v = v + std::to_string(version[v]);
                current_name[v] = phi_v;
                ssa_tac.emplace_back("phi", phi_v, then_v, else_v);
            }
            // Skip to end label
            i = end_label_idx;
            continue;
        }
        // Normal SSA for non-branch code
        std::string dest = instr.dest;
        std::string src1 = instr.src1;
        std::string src2 = instr.src2;
        if (!src1.empty() && current_name.count(src1)) src1 = current_name[src1];
        if (!src2.empty() && current_name.count(src2)) src2 = current_name[src2];
        if (!dest.empty()) {
            version[dest]++;
            current_name[dest] = dest + std::to_string(version[dest]);
            dest = current_name[dest];
        }
        ssa_tac.emplace_back(instr.op, dest, src1, src2);
        ++i;
    }
}

void print_ssa_tac_minimal() {
    std::cout << "\n=== SSA Generated (with phi functions where needed) ===" << std::endl;
    for (const auto& instr : ssa_tac) {
        if (instr.op == "phi") {
            std::cout << instr.dest << " = phi(" << instr.src1 << ", " << instr.src2 << ")" << std::endl;
        } else {
            // Print other SSA instructions for completeness
            if (instr.op == "label") {
                std::cout << instr.dest << ":" << std::endl;
            } else if (instr.op == "goto") {
                std::cout << "    goto " << instr.src1 << std::endl;
            } else if (instr.op == "ifz_goto") {
                std::cout << "    ifz " << instr.src1 << " goto " << instr.src2 << std::endl;
            } else if (instr.op == "print") {
                std::cout << "    print " << instr.src1 << std::endl;
            } else if (instr.op == "=" && instr.src2.empty()) {
                std::cout << "    " << instr.dest << " = " << instr.src1 << std::endl;
            } else {
                std::cout << "    " << instr.dest << " = " << instr.src1 << " " << instr.op << " " << instr.src2 << std::endl;
            }
        }
    }
    std::cout << "--------------------" << std::endl;
}

// --- AST Printing Helper ---
void print_ast_node(ASTNode* node, int indent = 0) {
    if (!node) return;
    std::string indent_str(indent * 2, ' ');
    std::cout << indent_str;

    // Use dynamic_cast to identify specific node types for printing
    if (auto* lit = dynamic_cast<Literal*>(node)) {
        std::cout << "Literal: " << lit->getToken().value << std::endl;
    } else if (auto* blit = dynamic_cast<BooleanLiteral*>(node)) {
        std::cout << "BooleanLiteral: " << blit->getToken().value << std::endl;
    } else if (auto* ident = dynamic_cast<IdentifierExpression*>(node)) {
        std::cout << "Identifier: " << ident->getIdentifier().value << std::endl;
    } else if (auto* assign = dynamic_cast<AssignmentExpression*>(node)) {
        std::cout << "Assignment: " << assign->getIdentifier().value << " =" << std::endl;
        print_ast_node(assign->getValue(), indent + 1);
    } else if (auto* varDecl = dynamic_cast<VariableDeclaration*>(node)) {
        std::cout << "VarDecl: " << varDecl->getIdentifier().value;
        if (varDecl->getInitializer()) {
            std::cout << " =" << std::endl;
            print_ast_node(varDecl->getInitializer(), indent + 1);
        } else {
            std::cout << std::endl;
        }
    } else if (auto* unary = dynamic_cast<UnaryExpression*>(node)) {
        std::cout << "Unary: " << unary->getOp().value << std::endl;
        print_ast_node(unary->getRight(), indent + 1);
    } else if (auto* bin = dynamic_cast<BinaryExpression*>(node)) {
        std::cout << "Binary: " << bin->getOp().value << std::endl;
        print_ast_node(bin->getLeft(), indent + 1);
        print_ast_node(bin->getRight(), indent + 1);
    } else if (auto* block = dynamic_cast<BlockStatement*>(node)) {
        std::cout << "Block {" << std::endl;
        for (auto* stmt : block->getStatements()) {
            print_ast_node(stmt, indent + 1);
        }
        std::cout << indent_str << "}" << std::endl;
    } else if (auto* ifstmt = dynamic_cast<IfStatement*>(node)) {
        std::cout << "If (" << std::endl;
        print_ast_node(ifstmt->getCondition(), indent + 1);
        std::cout << indent_str << ") Then {" << std::endl;
        print_ast_node(ifstmt->getThenBranch(), indent + 1);
        if (ifstmt->getElseBranch()) {
            std::cout << indent_str << "} Else {" << std::endl;
            print_ast_node(ifstmt->getElseBranch(), indent + 1);
        }
        std::cout << indent_str << "}" << std::endl;
    } else if (auto* printstmt = dynamic_cast<PrintStatement*>(node)) {
        std::cout << "Print (" << std::endl;
        print_ast_node(printstmt->getExpression(), indent + 1);
        std::cout << indent_str << ")" << std::endl;
    } else {
        // Fallback for any unhandled ASTNode types, though ideally all should be covered.
        std::cout << "Unhandled AST Node Type (Semantic Type): " << static_cast<int>(node->getType()) << std::endl;
    }
}

void print_ast(ASTNode* ast) {
    std::cout << "\n=== Abstract Syntax Tree (AST) ===" << std::endl;
    print_ast_node(ast);
    std::cout << "--------------------" << std::endl;
}

// --- Bytecode Printing Helper ---
void print_bytecode(const std::vector<Bytecode>& bytecode) {
    std::cout << "\n=== Bytecode ===" << std::endl;
    for (size_t i = 0; i < bytecode.size(); ++i) {
        const auto& instr = bytecode[i];
        std::cout << i << ": " << instruction_to_string(instr.instruction);
        // Only print operand if it's relevant for the instruction
        if (instr.instruction == Instruction::PUSH_INT ||
            instr.instruction == Instruction::PUSH_FLOAT ||
            instr.instruction == Instruction::PUSH_STRING ||
            instr.instruction == Instruction::JUMP ||
            instr.instruction == Instruction::JUMP_IF_FALSE ||
            instr.instruction == Instruction::JUMP_IF_TRUE ||
            instr.instruction == Instruction::LOAD ||
            instr.instruction == Instruction::STORE) {
            std::cout << " " << instr.operand;
        }
        std::cout << std::endl;
    }
    std::cout << "--------------------" << std::endl;
}

// --- AST Deep Copy Helper ---
ASTNode* deep_copy_ast(const ASTNode* node) {
    if (!node) return nullptr;
    using Type = ASTNode::Type;
    switch (node->getType()) {
        case Type::INTEGER:
        case Type::FLOAT:
        case Type::STRING_LITERAL:
        case Type::BOOLEAN_LITERAL: {
            // Copy literal or boolean literal
            if (auto* lit = dynamic_cast<const Literal*>(node))
                return new Literal(lit->getToken());
            if (auto* blit = dynamic_cast<const BooleanLiteral*>(node))
                return new BooleanLiteral(blit->getToken());
            break;
        }
        case Type::IDENTIFIER_EXPRESSION: {
            auto* ident = dynamic_cast<const IdentifierExpression*>(node);
            return new IdentifierExpression(ident->getIdentifier());
        }
        case Type::ASSIGNMENT_EXPRESSION: {
            auto* assign = dynamic_cast<const AssignmentExpression*>(node);
            return new AssignmentExpression(assign->getIdentifier(),
                dynamic_cast<Expression*>(deep_copy_ast(assign->getValue())));
        }
        case Type::VARIABLE_DECLARATION: {
            auto* varDecl = dynamic_cast<const VariableDeclaration*>(node);
            return new VariableDeclaration(varDecl->getIdentifier(),
                varDecl->getInitializer() ? dynamic_cast<Expression*>(deep_copy_ast(varDecl->getInitializer())) : nullptr);
        }
        case Type::UNARY_EXPRESSION: {
            auto* unary = dynamic_cast<const UnaryExpression*>(node);
            return new UnaryExpression(unary->getOp(),
                dynamic_cast<Expression*>(deep_copy_ast(unary->getRight())));
        }
        case Type::BLOCK_STATEMENT: {
            auto* block = dynamic_cast<const BlockStatement*>(node);
            std::vector<ASTNode*> stmts;
            for (auto* stmt : block->getStatements())
                stmts.push_back(deep_copy_ast(stmt));
            return new BlockStatement(stmts);
        }
        case Type::IF_STATEMENT: {
            auto* ifstmt = dynamic_cast<const IfStatement*>(node);
            return new IfStatement(
                dynamic_cast<Expression*>(deep_copy_ast(ifstmt->getCondition())),
                deep_copy_ast(ifstmt->getThenBranch()),
                ifstmt->getElseBranch() ? deep_copy_ast(ifstmt->getElseBranch()) : nullptr
            );
        }
        case Type::PRINT_STATEMENT: {
            auto* printstmt = dynamic_cast<const PrintStatement*>(node);
            return new PrintStatement(dynamic_cast<Expression*>(deep_copy_ast(printstmt->getExpression())));
        }
        default:
            break;
    }
    return nullptr; // Should not happen
}

// === PHASE 6: CODE OPTIMIZATION ===
// Simple constant folding: replace constant binary expressions with a literal
ASTNode* constant_fold(ASTNode* ast) {
    if (!ast) { std::cerr << "constant_fold called with nullptr!" << std::endl; return nullptr; }
    ASTNode* result = nullptr;
    if (auto* bin = dynamic_cast<BinaryExpression*>(ast)) {
        auto* left = constant_fold(bin->getLeft());
        auto* right = constant_fold(bin->getRight());
        auto* left_lit = dynamic_cast<Literal*>(left);
        auto* right_lit = dynamic_cast<Literal*>(right);
        if (left_lit && right_lit) {
            int l = std::stoi(left_lit->getToken().value);
            int r = std::stoi(right_lit->getToken().value);
            int result_val = 0;
            if (bin->getOp().type == TokenType::PLUS) result_val = l + r;
            else if (bin->getOp().type == TokenType::MINUS) result_val = l - r;
            else if (bin->getOp().type == TokenType::STAR) result_val = l * r;
            else if (bin->getOp().type == TokenType::SLASH && r != 0) result_val = l / r;
            else {
                delete left; delete right;
                result = deep_copy_ast(bin); // Don't fold if not supported
                return result;
            }
            delete left; delete right;
            result = new Literal(Token(TokenType::INT_LITERAL, std::to_string(result_val), 0, 0));
            return result;
        }
        result = new BinaryExpression(dynamic_cast<Expression*>(left), bin->getOp(), dynamic_cast<Expression*>(right));
    } else if (auto* unary = dynamic_cast<UnaryExpression*>(ast)) {
        auto* right = constant_fold(unary->getRight());
        auto* right_lit = dynamic_cast<Literal*>(right);
        if (right_lit && unary->getOp().type == TokenType::MINUS) {
            int val = std::stoi(right_lit->getToken().value);
            delete right;
            result = new Literal(Token(TokenType::INT_LITERAL, std::to_string(-val), 0, 0));
            return result;
        }
        result = new UnaryExpression(unary->getOp(), dynamic_cast<Expression*>(right));
    } else if (auto* block = dynamic_cast<BlockStatement*>(ast)) {
        std::vector<ASTNode*> stmts;
        for (auto* stmt : block->getStatements()) {
            ASTNode* folded = constant_fold(stmt);
            if (folded) {
                stmts.push_back(folded);
            } else {
                stmts.push_back(deep_copy_ast(stmt));
            }
        }
        result = new BlockStatement(stmts);
    } else if (auto* ifstmt = dynamic_cast<IfStatement*>(ast)) {
        result = new IfStatement(
            dynamic_cast<Expression*>(constant_fold(ifstmt->getCondition())),
            constant_fold(ifstmt->getThenBranch()),
            ifstmt->getElseBranch() ? constant_fold(ifstmt->getElseBranch()) : nullptr
        );
    } else if (auto* printstmt = dynamic_cast<PrintStatement*>(ast)) {
        result = new PrintStatement(dynamic_cast<Expression*>(constant_fold(printstmt->getExpression())));
    } else if (auto* varDecl = dynamic_cast<VariableDeclaration*>(ast)) {
        Expression* folded_init = nullptr;
        if (varDecl->getInitializer()) {
            folded_init = dynamic_cast<Expression*>(constant_fold(varDecl->getInitializer()));
            if (!folded_init) {
                folded_init = dynamic_cast<Expression*>(deep_copy_ast(varDecl->getInitializer()));
            }
        }
        result = new VariableDeclaration(varDecl->getIdentifier(), folded_init);
    } else if (auto* assign = dynamic_cast<AssignmentExpression*>(ast)) {
        Expression* folded_value = nullptr;
        if (assign->getValue()) {
            folded_value = dynamic_cast<Expression*>(constant_fold(assign->getValue()));
            if (!folded_value) {
                folded_value = dynamic_cast<Expression*>(deep_copy_ast(assign->getValue()));
            }
        }
        result = new AssignmentExpression(assign->getIdentifier(), folded_value);
    }
    // At the end of constant_fold, if result is still nullptr, return a deep copy of the original node
    if (!result) {
        result = deep_copy_ast(ast);
    }
    return result;
}

// === PHASE 1: PREPROCESSING ===
// Remove single-line comments (// ...)
std::string preprocess(const std::string& source_code) {
    std::cout << "\n=== PHASE 1: PREPROCESSING ===" << std::endl;
    std::cout << "Removing comments from the code..." << std::endl;
    std::string no_comments;
    std::regex comment_pattern("//.*");
    std::istringstream iss(source_code);
    std::string line;
    while (std::getline(iss, line)) {
        line = std::regex_replace(line, comment_pattern, "");
        no_comments += line + "\n";
    }
    return no_comments;
}

// === PHASE 4: SEMANTIC ANALYSIS ===
// Walk the AST and check for undeclared/duplicate variables and type errors
bool semantic_check(ASTNode* ast, std::unordered_set<std::string>& declared_vars) {
    if (!ast) return true;
    using Type = ASTNode::Type;
    if (auto* varDecl = dynamic_cast<VariableDeclaration*>(ast)) {
        std::string name = varDecl->getIdentifier().value;
        if (declared_vars.count(name)) {
            std::cerr << "Semantic Error: Duplicate variable '" << name << "'" << std::endl;
            return false;
        }
        declared_vars.insert(name);
        if (varDecl->getInitializer()) {
            return semantic_check(varDecl->getInitializer(), declared_vars);
        }
    } else if (auto* assign = dynamic_cast<AssignmentExpression*>(ast)) {
        std::string name = assign->getIdentifier().value;
        if (!declared_vars.count(name)) {
            std::cerr << "Semantic Error: Assignment to undeclared variable '" << name << "'" << std::endl;
            return false;
        }
        return semantic_check(assign->getValue(), declared_vars);
    } else if (auto* ident = dynamic_cast<IdentifierExpression*>(ast)) {
        std::string name = ident->getIdentifier().value;
        if (!declared_vars.count(name)) {
            std::cerr << "Semantic Error: Use of undeclared variable '" << name << "'" << std::endl;
            return false;
        }
    } else if (auto* block = dynamic_cast<BlockStatement*>(ast)) {
        for (auto* stmt : block->getStatements()) {
            if (!semantic_check(stmt, declared_vars)) return false;
        }
    } else if (auto* ifstmt = dynamic_cast<IfStatement*>(ast)) {
        if (!semantic_check(ifstmt->getCondition(), declared_vars)) return false;
        if (!semantic_check(ifstmt->getThenBranch(), declared_vars)) return false;
        if (ifstmt->getElseBranch() && !semantic_check(ifstmt->getElseBranch(), declared_vars)) return false;
    } else if (auto* printstmt = dynamic_cast<PrintStatement*>(ast)) {
        if (!semantic_check(printstmt->getExpression(), declared_vars)) return false;
    } else if (auto* bin = dynamic_cast<BinaryExpression*>(ast)) {
        if (!semantic_check(bin->getLeft(), declared_vars)) return false;
        if (!semantic_check(bin->getRight(), declared_vars)) return false;
    } else if (auto* unary = dynamic_cast<UnaryExpression*>(ast)) {
        if (!semantic_check(unary->getRight(), declared_vars)) return false;
    }
    return true;
}

// This function takes source code as input and runs it through all compiler phases.
void run_compiler(const std::string& source_code) {
    // Reset global/static state for each file
    tac_instructions.clear();
    tac_temp_counter = 0;
    ssa_tac.clear();
    std::cout << "Starting run_compiler" << std::endl;
    // === PHASE 1: PREPROCESSING ===
    std::cout << "\n=== PHASE 1: PREPROCESSING ===" << std::endl;
    std::string preprocessed = preprocess(source_code);
    std::cout << "Preprocessing complete." << std::endl;

    // === PHASE 2: LEXICAL ANALYSIS ===
    std::cout << "\n=== PHASE 2: LEXICAL ANALYSIS ===" << std::endl;
    Lexer lexer(preprocessed);
    std::vector<Token> tokens = lexer.tokenize();
    print_tokens(tokens); // Print tokens
    std::cout << "Lexical analysis complete." << std::endl;

    // === PHASE 3: SYNTAX ANALYSIS ===
    std::cout << "\n=== PHASE 3: SYNTAX ANALYSIS ===" << std::endl;
    Parser parser(tokens);
    ASTNode* ast = parser.parse();
    if (ast) {
        print_ast(ast); // Print initial AST
        std::cout << "Syntax analysis complete. AST generated." << std::endl;
    } else {
        std::cout << "Syntax analysis failed. AST generation failed." << std::endl;
        return;
    }

    // === PHASE 4: SEMANTIC ANALYSIS ===
    std::cout << "\n=== PHASE 4: SEMANTIC ANALYSIS ===" << std::endl;
    std::unordered_set<std::string> declared_vars;
    if (!semantic_check(ast, declared_vars)) {
        std::cout << "Semantic analysis complete. Semantic errors found." << std::endl;
        delete ast;
        return;
    }
    std::cout << "Semantic analysis complete. Semantic analysis passed." << std::endl;

    // === TAC GENERATION ===
    std::cout << "\n=== TAC GENERATION ===" << std::endl;
    tac_instructions.clear();
    tac_temp_counter = 0;
    generate_tac(ast);
    print_tac(); // Print TAC
    std::cout << "TAC generation complete." << std::endl;

    // === CFG GENERATION ===
    std::cout << "\n=== CFG GENERATION ===" << std::endl;
    auto cfg = build_cfg(tac_instructions);
    print_cfg(cfg); // Print CFG
    std::cout << "CFG generation complete." << std::endl;

    // === SSA GENERATION (with phi) ===" << std::endl;
    std::cout << "\n=== SSA GENERATION (with phi) ===" << std::endl;
    to_ssa_with_phi(tac_instructions);
    print_ssa_tac_minimal(); // Print SSA
    std::cout << "SSA generation complete." << std::endl;

    // === PHASE 5: INTERMEDIATE CODE GENERATION ===
    std::cout << "\n=== PHASE 5: INTERMEDIATE CODE GENERATION ===" << std::endl;
    Compiler original_compiler; // Use a distinct name for the original compiler
    std::vector<Bytecode> bytecode = original_compiler.compile(ast);
    std::cout << "Intermediate code generation complete. Bytecode generated." << std::endl;
    print_bytecode(bytecode); // Always print initial bytecode

    // === PHASE 6: CODE OPTIMIZATION ===
    std::cout << "\n=== PHASE 6: CODE OPTIMIZATION ===" << std::endl;
    ASTNode* optimized_ast = constant_fold(ast);
    Compiler* active_compiler = &original_compiler; // Pointer to the compiler instance whose string literals will be used
    Compiler* optimized_compiler_ptr = nullptr; // Pointer for dynamically allocated optimized compiler

    if (optimized_ast != ast) {
        std::cout << "Optimized AST generated." << std::endl;
        print_ast(optimized_ast); // Print optimized AST

        // Create a new compiler instance for optimized AST dynamically
        optimized_compiler_ptr = new Compiler();
        bytecode = optimized_compiler_ptr->compile(optimized_ast);
        const auto& sem_errors = optimized_compiler_ptr->getSemanticErrors();
        if (!sem_errors.empty()) {
            std::cerr << "Semantic errors after optimization:" << std::endl;
            for (const auto& err : sem_errors) std::cerr << err << std::endl;
        }
        std::cout << "Optimized bytecode generated." << std::endl;
        print_bytecode(bytecode); // Print optimized bytecode
        std::cout << "Code optimization complete." << std::endl;
        active_compiler = optimized_compiler_ptr; // Update active_compiler to the dynamically allocated one
    } else {
        std::cout << "Code optimization complete. No optimizations applied." << std::endl;
    }

    // === PHASE 7: CODE GENERATION ===
    std::cout << "\n=== PHASE 7: CODE GENERATION ===" << std::endl;
    std::cout << "Code generation complete." << std::endl;
    // === PHASE 8: EXECUTION ===" << std::endl;
    std::cout << "\n=== PHASE 8: EXECUTION ===" << std::endl;
    VM vm;
    double result = vm.run(bytecode, active_compiler->getStringLiterals());
    std::cout << "Execution complete." << std::endl;

    delete ast;
    if (optimized_ast != ast) delete optimized_ast;
    if (optimized_compiler_ptr) delete optimized_compiler_ptr; // Clean up dynamically allocated compiler
    std::cout << "Finished run_compiler" << std::endl;
    std::cout << std::flush;
}

int main(int argc, char* argv[]) {
    std::cout << "Welcome to our Simple Compiler!" << std::endl;
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            std::cout << "Main loop, file: " << arg << std::endl;
            if (arg.length() > 6 && arg.substr(arg.length() - 6) == ".cocom") {
                std::ifstream file(arg);
                if (file.is_open()) {
                    std::string file_content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                    std::cout << "\n==============================" << std::endl;
                    std::cout << "Processing file: " << arg << std::endl;
                    std::cout << "==============================" << std::endl;
                    std::cout << "About to call run_compiler for: " << arg << std::endl;
                    run_compiler(file_content);
                    std::cout << "Finished run_compiler for: " << arg << std::endl;
                    file.close();
                } else {
                    std::cerr << "Error: Could not open file '" << arg << "'" << std::endl;
                }
            } else {
                std::cerr << "Error: Invalid argument. Expected a .cocom file path." << std::endl;
            }
        }
    } else {
        std::cout << "Enter code (type 'exit' to quit, 'clear' to clear screen, 'help' for commands):" << std::endl;
        std::string line;
        while (true) {
            std::cout << "> ";
            std::getline(std::cin, line);
            if (line == "exit") break;
            if (line == "clear") {
#ifdef _WIN32
                system("cls");
#else
                system("clear");
#endif
                continue;
            }
            if (line == "help") {
                std::cout << "\nAvailable commands:\n";
                std::cout << "  exit   - Quit the compiler\n";
                std::cout << "  clear  - Clear the screen\n";
                std::cout << "  help   - Show this help message\n";
                std::cout << "  :begin - Start multi-line input mode (end with :end)\n";
                std::cout << "Type code to compile and run it immediately.\n";
                std::cout << "To enter multi-line code, type :begin, then your code, then :end on a new line.\n";
                continue;
            }
            if (line == ":begin") {
                std::cout << "(Multi-line mode. Type :end on a new line to finish.)" << std::endl;
                std::string multiline, mline;
                while (true) {
                    std::getline(std::cin, mline);
                    if (mline == ":end") break;
                    multiline += mline + "\n";
                }
                run_compiler(multiline);
                continue;
            }
            if (!line.empty()) run_compiler(line);
        }
    }
    return 0;
}
