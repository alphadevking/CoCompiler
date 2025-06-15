#ifndef BYTECODE_H
#define BYTECODE_H

#include "Tokens.h"

// --- VM Instructions ---
enum class Instruction {
    PUSH_INT = 0,   // Push an integer literal onto the stack
    PUSH_FLOAT = 1, // Push a floating-point literal onto the stack
    ADD = 2,        // Pop two values, add them, push result
    SUB = 3,        // Pop two values, subtract them, push result
    MUL = 4,        // Pop two values, multiply them, push result
    DIV = 5,        // Pop two values, divide them, push result
    NEGATE = 6,     // Pop one value, negate it, push result (for unary minus)
    POP = 7,        // Pop one value from the stack
    STORE = 8,      // Pop value, pop address, store value at address (simplified for now)
    LOAD = 9,       // Pop address, push value from address (simplified for now)
    HALT = 10,      // Stop execution

    // Control flow instructions
    JUMP_IF_FALSE = 11, // Pop value, if false, jump to address
    JUMP = 12,          // Unconditional jump to address
    JUMP_IF_TRUE = 13,  // Pop value, if true (non-zero), jump to address

    // Comparison and Logical instructions
    GREATER = 14,       // Pop two values, push 1 if left > right, else 0
    LESS = 15,          // Pop two values, push 1 if left < right, else 0
    GREATER_EQUAL = 16, // Pop two values, push 1 if left >= right, else 0
    LESS_EQUAL = 17,    // Pop two values, push 1 if left <= right, else 0
    EQUAL_EQUAL = 18,   // Pop two values, push 1 if left == right, else 0
    BANG_EQUAL = 19,    // Pop two values, push 1 if left != right, else 0
    NOT = 20,           // Pop value, push 1 if 0, else 0 (logical NOT)
    AND = 21,           // Logical AND (short-circuiting handled by JUMP_IF_FALSE)
    OR = 22,            // Logical OR (short-circuiting handled by JUMP_IF_TRUE)

    // Other instructions
    PUSH_STRING = 23,    // Push a string literal index onto the stack
    CONCAT_STRING = 24,   // Pop two string indices, concatenate strings, push new string index
    PRINT_VALUE = 25,     // Pop value from stack and print it (number or boolean)
    PRINT_STRING = 26     // Pop string index from stack and print the string literal
};

/**
 * @brief Converts an Instruction enum value to its string representation.
 * @param instr The Instruction enum value.
 * @return The string representation of the instruction.
 */
inline std::string instruction_to_string(Instruction instr) {
    switch (instr) {
        case Instruction::PUSH_INT: return "PUSH_INT";
        case Instruction::PUSH_FLOAT: return "PUSH_FLOAT";
        case Instruction::ADD: return "ADD";
        case Instruction::SUB: return "SUB";
        case Instruction::MUL: return "MUL";
        case Instruction::DIV: return "DIV";
        case Instruction::NEGATE: return "NEGATE";
        case Instruction::POP: return "POP";
        case Instruction::STORE: return "STORE";
        case Instruction::LOAD: return "LOAD";
        case Instruction::HALT: return "HALT";
        case Instruction::JUMP_IF_FALSE: return "JUMP_IF_FALSE";
        case Instruction::JUMP: return "JUMP";
        case Instruction::JUMP_IF_TRUE: return "JUMP_IF_TRUE";
        case Instruction::GREATER: return "GREATER";
        case Instruction::LESS: return "LESS";
        case Instruction::GREATER_EQUAL: return "GREATER_EQUAL";
        case Instruction::LESS_EQUAL: return "LESS_EQUAL";
        case Instruction::EQUAL_EQUAL: return "EQUAL_EQUAL";
        case Instruction::BANG_EQUAL: return "BANG_EQUAL";
        case Instruction::NOT: return "NOT";
        case Instruction::AND: return "AND";
        case Instruction::OR: return "OR";
        case Instruction::PUSH_STRING: return "PUSH_STRING";
        case Instruction::CONCAT_STRING: return "CONCAT_STRING";
        case Instruction::PRINT_VALUE: return "PRINT_VALUE";
        case Instruction::PRINT_STRING: return "PRINT_STRING";
        default: return "UNKNOWN";
    }
}

struct Bytecode {
    Instruction instruction;
    double operand; // For jump targets, literal values, or memory addresses (now supports float directly)

    // Constructor for instructions without an explicit operand (e.g., ADD, HALT)
    Bytecode(Instruction instruction) : instruction(instruction), operand(0.0) {}

    // Constructor for instructions with an integer operand (e.g., PUSH_INT, JUMP, JUMP_IF_FALSE, STORE, LOAD)
    Bytecode(Instruction instruction, int intOperand) : instruction(instruction), operand(static_cast<double>(intOperand)) {}

    // Constructor for instructions with a float operand (e.g., PUSH_FLOAT)
    Bytecode(Instruction instruction, float floatOperand) : instruction(instruction), operand(static_cast<double>(floatOperand)) {
        // Now storing float directly as double operand, preserving precision.
    }
};

#endif // BYTECODE_H
