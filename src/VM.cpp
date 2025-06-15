#include "VM.h"
#include <iostream>

/**
 * @brief Constructs a new VM object.
 * Initializes the program counter.
 */
VM::VM() : pc(0) {}

/**
 * @brief Runs the provided bytecode instructions.
 * @param bytecode A vector of Bytecode instructions to execute.
 * @param string_literals A vector of string literals from the compiler.
 * @return The final value on the stack if the program halts, or -1 in case of an error.
 */
double VM::run(const std::vector<Bytecode>& bytecode, const std::vector<std::string>& string_literals) {
    this->bytecode = bytecode;
    this->string_literals = string_literals; // Store the string literals
    stack.clear();
    memory.clear(); // Clear memory for a new run
    pc = 0;

    while (pc < this->bytecode.size()) {
        Bytecode instruction = this->bytecode[pc]; // Peek at instruction
        std::cout << "DEBUG: PC: " << pc << ", Instruction: " << static_cast<int>(instruction.instruction)
                  << " (" << instruction_to_string(instruction.instruction) << ")";
        if (instruction.instruction == Instruction::PUSH_INT ||
            instruction.instruction == Instruction::PUSH_FLOAT ||
            instruction.instruction == Instruction::PUSH_STRING ||
            instruction.instruction == Instruction::JUMP ||
            instruction.instruction == Instruction::JUMP_IF_FALSE ||
            instruction.instruction == Instruction::JUMP_IF_TRUE) {
            std::cout << " Operand: " << instruction.operand;
        }
        std::cout << " Stack: [";
        for (size_t i = 0; i < stack.size(); ++i) {
            std::cout << stack[i] << (i == stack.size() - 1 ? "" : ", ");
        }
        std::cout << "]" << std::endl;

        pc++; // Then increment pc

        switch (instruction.instruction) {
            case Instruction::PUSH_INT:
                stack.push_back(static_cast<double>(instruction.operand));
                break;
            case Instruction::PUSH_FLOAT:
                stack.push_back(static_cast<double>(instruction.operand));
                break;
            case Instruction::ADD: {
                if (stack.size() < 2) { std::cerr << "VM Error: Stack underflow for ADD." << std::endl; return -1; }
                double val2 = stack.back(); stack.pop_back();
                double val1 = stack.back(); stack.pop_back();
                stack.push_back(val1 + val2);
                break;
            }
            case Instruction::SUB: {
                if (stack.size() < 2) { std::cerr << "VM Error: Stack underflow for SUB." << std::endl; return -1; }
                double val2 = stack.back(); stack.pop_back();
                double val1 = stack.back(); stack.pop_back();
                stack.push_back(val1 - val2);
                break;
            }
            case Instruction::MUL: {
                if (stack.size() < 2) { std::cerr << "VM Error: Stack underflow for MUL." << std::endl; return -1; }
                double val2 = stack.back(); stack.pop_back();
                double val1 = stack.back(); stack.pop_back();
                stack.push_back(val1 * val2);
                break;
            }
            case Instruction::DIV: {
                if (stack.size() < 2) { std::cerr << "VM Error: Stack underflow for DIV." << std::endl; return -1; }
                double val2 = stack.back(); stack.pop_back();
                if (val2 == 0.0) { std::cerr << "VM Error: Division by zero." << std::endl; return -1; }
                double val1 = stack.back(); stack.pop_back();
                stack.push_back(val1 / val2);
                break;
            }
            case Instruction::NEGATE: {
                if (stack.empty()) { std::cerr << "VM Error: Stack underflow for NEGATE." << std::endl; return -1; }
                double val = stack.back(); stack.pop_back();
                stack.push_back(-val);
                break;
            }
            case Instruction::POP: {
                if (stack.empty()) { std::cerr << "VM Error: Stack underflow for POP." << std::endl; return -1; }
                stack.pop_back();
                break;
            }
            case Instruction::STORE: {
                if (stack.size() < 2) { std::cerr << "VM Error: Stack underflow for STORE." << std::endl; return -1; }
                int address = static_cast<int>(stack.back()); stack.pop_back();
                double value = stack.back(); stack.pop_back();

                if (address < 0) {
                    std::cerr << "VM Error: Invalid memory address for STORE: " << address << std::endl;
                    return -1;
                }
                if (address >= memory.size()) {
                    memory.resize(address + 1);
                }
                memory[address] = value;
                // Push the stored value back onto the stack for assignment expressions
                stack.push_back(value);
                break;
            }
            case Instruction::LOAD: {
                if (stack.size() < 1) { std::cerr << "VM Error: Stack underflow for LOAD." << std::endl; return -1; }
                int address = static_cast<int>(stack.back()); stack.pop_back();

                if (address < 0 || address >= memory.size()) {
                    std::cerr << "VM Error: Invalid memory address for LOAD: " << address << std::endl;
                    return -1;
                }
                stack.push_back(memory[address]);
                break;
            }
            case Instruction::HALT:
                if (stack.empty()) {
                    return 0;
                }
                return stack.back();
            case Instruction::JUMP_IF_FALSE: {
                if (stack.empty()) { std::cerr << "VM Error: Stack underflow for JUMP_IF_FALSE." << std::endl; return -1; }
                double condition = stack.back(); stack.pop_back();
                if (condition == 0.0) {
                    pc = static_cast<int>(instruction.operand);
                }
                break;
            }
            case Instruction::JUMP: {
                pc = static_cast<int>(instruction.operand);
                break;
            }
            case Instruction::JUMP_IF_TRUE: {
                if (stack.empty()) { std::cerr << "VM Error: Stack underflow for JUMP_IF_TRUE." << std::endl; return -1; }
                double condition = stack.back(); stack.pop_back();
                if (condition != 0.0) {
                    pc = static_cast<int>(instruction.operand);
                }
                break;
            }
            case Instruction::GREATER: {
                if (stack.size() < 2) { std::cerr << "VM Error: Stack underflow for GREATER." << std::endl; return -1; }
                double val2 = stack.back(); stack.pop_back();
                double val1 = stack.back(); stack.pop_back();
                bool result = (val1 > val2);
                stack.push_back(result ? 1.0 : 0.0);
                break;
            }
            case Instruction::LESS: {
                if (stack.size() < 2) { std::cerr << "VM Error: Stack underflow for LESS." << std::endl; return -1; }
                double val2 = stack.back(); stack.pop_back();
                double val1 = stack.back(); stack.pop_back();
                bool result = (val1 < val2);
                stack.push_back(result ? 1.0 : 0.0);
                break;
            }
            case Instruction::GREATER_EQUAL: {
                if (stack.size() < 2) { std::cerr << "VM Error: Stack underflow for GREATER_EQUAL." << std::endl; return -1; }
                double val2 = stack.back(); stack.pop_back();
                double val1 = stack.back(); stack.pop_back();
                bool result = (val1 >= val2);
                stack.push_back(result ? 1.0 : 0.0);
                break;
            }
            case Instruction::LESS_EQUAL: {
                if (stack.size() < 2) { std::cerr << "VM Error: Stack underflow for LESS_EQUAL." << std::endl; return -1; }
                double val2 = stack.back(); stack.pop_back();
                double val1 = stack.back(); stack.pop_back();
                bool result = (val1 <= val2);
                stack.push_back(result ? 1.0 : 0.0);
                break;
            }
            case Instruction::EQUAL_EQUAL: {
                if (stack.size() < 2) { std::cerr << "VM Error: Stack underflow for EQUAL_EQUAL." << std::endl; return -1; }
                double val2 = stack.back(); stack.pop_back();
                double val1 = stack.back(); stack.pop_back();
                bool result = (val1 == val2);
                stack.push_back(result ? 1.0 : 0.0);
                break;
            }
            case Instruction::BANG_EQUAL: {
                if (stack.size() < 2) { std::cerr << "VM Error: Stack underflow for BANG_EQUAL." << std::endl; return -1; }
                double val2 = stack.back(); stack.pop_back();
                double val1 = stack.back(); stack.pop_back();
                bool result = (val1 != val2);
                stack.push_back(result ? 1.0 : 0.0);
                break;
            }
            case Instruction::NOT: {
                if (stack.empty()) { std::cerr << "VM Error: Stack underflow for NOT." << std::endl; return -1; }
                double val = stack.back(); stack.pop_back();
                bool result = (val == 0.0);
                stack.push_back(result ? 1.0 : 0.0);
                break;
            }
            case Instruction::AND:
            case Instruction::OR:
                std::cerr << "VM Error: Encountered logical operator instruction (AND/OR) directly. This should be handled by jumps." << std::endl;
                return -1;
            case Instruction::PUSH_STRING: { // PUSH_STRING is now 23
                stack.push_back(static_cast<double>(instruction.operand));
                break;
            }
            case Instruction::CONCAT_STRING: { // CONCAT_STRING is now 24
                if (stack.size() < 2) { std::cerr << "VM Error: Stack underflow for CONCAT_STRING." << std::endl; return -1; }
                int string_idx2 = static_cast<int>(stack.back()); stack.pop_back();
                int string_idx1 = static_cast<int>(stack.back()); stack.pop_back();

                if (string_idx1 < 0 || string_idx1 >= this->string_literals.size() ||
                    string_idx2 < 0 || string_idx2 >= this->string_literals.size()) {
                    std::cerr << "VM Error: Invalid string literal index for CONCAT_STRING." << std::endl;
                    return -1;
                }

                std::string concatenated_string = this->string_literals[string_idx1] + this->string_literals[string_idx2];

                int new_string_index = static_cast<int>(this->string_literals.size());
                this->string_literals.push_back(concatenated_string);
                stack.push_back(static_cast<double>(new_string_index));
                break;
            }
            case Instruction::PRINT_VALUE: { // New PRINT_VALUE instruction (25)
                if (stack.empty()) { std::cerr << "VM Error: Stack underflow for PRINT_VALUE." << std::endl; return -1; }
                double val = stack.back(); stack.pop_back();
                if (val == 0.0) {
                    std::cout << "false" << std::endl;
                } else if (val == 1.0) {
                    std::cout << "true" << std::endl;
                } else {
                    std::cout << val << std::endl;
                }
                break;
            }
            case Instruction::PRINT_STRING: { // New PRINT_STRING instruction (26)
                if (stack.empty()) { std::cerr << "VM Error: Stack underflow for PRINT_STRING." << std::endl; return -1; }
                int string_idx = static_cast<int>(stack.back()); stack.pop_back();
                if (string_idx < 0 || string_idx >= this->string_literals.size()) {
                    std::cerr << "VM Error: Invalid string literal index for PRINT_STRING." << std::endl;
                    return -1;
                }
                std::cout << this->string_literals[string_idx] << std::endl;
                break;
            }
            default:
                std::cerr << "VM Error: Unknown instruction: " << static_cast<int>(instruction.instruction) << std::endl;
                return -1;
        }
    }

    std::cerr << "VM Error: Program did not halt. Missing HALT instruction or infinite loop." << std::endl;
    return -1;
}
