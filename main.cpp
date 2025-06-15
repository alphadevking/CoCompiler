#include <iostream>
#include <string>
#include <vector>
#include <fstream> // For reading from file

#include "Tokens.h"
#include "src/Lexer.h"
#include "src/Parser.h"
#include "include/AST.h"
#include "src/Compiler.h"
#include "src/VM.h"

// Function to process a single source code string
void process_source_code(const std::string& source_code) {
    // Lexical Analysis (Scanning)
    std::cout << "\n========================================" << std::endl;
    std::cout << "Phase: Lexical Analysis (Scanning)" << std::endl;
    std::cout << "Explanation: Converts source code into a stream of tokens." << std::endl;
    std::cout << "Using: Lexer (src/Lexer.cpp, src/Lexer.h) to produce a stream of Token objects." << std::endl;
    std::cout << "========================================" << std::endl;

    Lexer lexer(source_code);
    std::vector<Token> tokens = lexer.tokenize();

    std::cout << "\n--- Tokens ---" << std::endl;
    for (const auto& token : tokens) {
        std::cout << token.toString() << std::endl;
    }
    std::cout << "-------------" << std::endl;

    // Syntax Analysis (Parsing)
    std::cout << "\n========================================" << std::endl;
    std::cout << "Phase: Syntax Analysis (Parsing)" << std::endl;
    std::cout << "Explanation: Parses the token stream to build an Abstract Syntax Tree (AST)." << std::endl;
    std::cout << "Using: Parser (src/Parser.cpp, src/Parser.h) to construct an ASTNode representation." << std::endl;
    std::cout << "========================================" << std::endl;

    Parser parser(tokens);
    ASTNode* ast = parser.parse();

    std::cout << "\n--- AST ---" << std::endl;
    if (ast != nullptr) {
        std::cout << ast->toString() << std::endl;
    } else {
        std::cout << "AST is null (parsing failed or empty input)" << std::endl;
    }
    std::cout << "-----------" << std::endl;

    // Semantic Analysis
    std::cout << "\n========================================" << std::endl;
    std::cout << "Phase: Semantic Analysis" << std::endl;
    std::cout << "Explanation: Checks for meaning and consistency, including type checking (int, float, string, bool) and variable declaration." << std::endl;
    std::cout << "Using: Compiler (src/Compiler.cpp) for checks, and Symbol Table (src/SymbolTable.cpp, include/SymbolTable.h) for identifier information and types." << std::endl;
    std::cout << "========================================" << std::endl;

    Compiler compiler;
    std::vector<Bytecode> bytecode_instructions = compiler.compile(ast);

    // Intermediate Code Generation
    std::cout << "\n========================================" << std::endl;
    std::cout << "Phase: Intermediate Code Generation" << std::endl;
    std::cout << "Explanation: Translates the validated code into an intermediate representation." << std::endl;
    std::cout << "Using: Compiler (src/Compiler.cpp) to generate Bytecode (include/Bytecode.h) as the intermediate representation." << std::endl;
    std::cout << "========================================" << std::endl;

    std::cout << "\n--- Instructions ---" << std::endl;
    if (!bytecode_instructions.empty()) {
        for (const auto& bytecode : bytecode_instructions) {
            switch (bytecode.instruction) {
                case Instruction::PUSH_INT: std::cout << "PUSH_INT " << static_cast<int>(bytecode.operand) << std::endl; break;
                case Instruction::PUSH_FLOAT: std::cout << "PUSH_FLOAT " << bytecode.operand << std::endl; break; // operand is already double
                case Instruction::PUSH_STRING: std::cout << "PUSH_STRING " << static_cast<int>(bytecode.operand) << " (\"" << compiler.getStringLiteral(static_cast<int>(bytecode.operand)) << "\")" << std::endl; break; // New: PUSH_STRING instruction
                case Instruction::ADD: std::cout << "ADD" << std::endl; break;
                case Instruction::SUB: std::cout << "SUB" << std::endl; break;
                case Instruction::MUL: std::cout << "MUL" << std::endl; break;
                case Instruction::DIV: std::cout << "DIV" << std::endl; break;
                case Instruction::STORE: std::cout << "STORE" << std::endl; break;
                case Instruction::LOAD: std::cout << "LOAD" << std::endl; break;
                case Instruction::JUMP_IF_FALSE: std::cout << "JUMP_IF_FALSE " << static_cast<int>(bytecode.operand) << std::endl; break;
                case Instruction::JUMP: std::cout << "JUMP " << static_cast<int>(bytecode.operand) << std::endl; break;
                case Instruction::JUMP_IF_TRUE: std::cout << "JUMP_IF_TRUE " << static_cast<int>(bytecode.operand) << std::endl; break; // New: JUMP_IF_TRUE instruction
                case Instruction::GREATER: std::cout << "GREATER" << std::endl; break; // New: GREATER instruction
                case Instruction::LESS: std::cout << "LESS" << std::endl; break; // New: LESS instruction
                case Instruction::GREATER_EQUAL: std::cout << "GREATER_EQUAL" << std::endl; break; // New: GREATER_EQUAL instruction
                case Instruction::LESS_EQUAL: std::cout << "LESS_EQUAL" << std::endl; break; // New: LESS_EQUAL instruction
                case Instruction::EQUAL_EQUAL: std::cout << "EQUAL_EQUAL" << std::endl; break; // New: EQUAL_EQUAL instruction
                case Instruction::BANG_EQUAL: std::cout << "BANG_EQUAL" << std::endl; break;
                case Instruction::NOT: std::cout << "NOT" << std::endl; break; // Added NOT instruction for printing
                case Instruction::AND: std::cout << "AND" << std::endl; break;
                case Instruction::OR: std::cout << "OR" << std::endl; break;
                case Instruction::PRINT_VALUE: std::cout << "PRINT_VALUE" << std::endl; break; // New: PRINT_VALUE instruction
                case Instruction::PRINT_STRING: std::cout << "PRINT_STRING " << static_cast<int>(bytecode.operand) << " (\"" << compiler.getStringLiteral(static_cast<int>(bytecode.operand)) << "\")" << std::endl; break; // New: PRINT_STRING instruction
                case Instruction::HALT: std::cout << "HALT" << std::endl; break;
                case Instruction::POP: std::cout << "POP" << std::endl; break;
                default: std::cout << "UNKNOWN INSTRUCTION: " << static_cast<int>(bytecode.instruction) << std::endl; break;
            }
        }
        std::cout << "----------------------" << std::endl;
    } else {
        std::cout << "No instructions to execute (compilation failed)" << std::endl;
    }

    VM vm;
    double result = 0;
    if (!bytecode_instructions.empty()) {
        result = vm.run(bytecode_instructions, compiler.getStringLiterals()); // Pass string literals to VM
    } else {
        std::cout << "VM not run due to empty bytecode." << std::endl;
    }

    std::cout << "\n--- Result ---" << std::endl;
    // --- NEW IMPLEMENTATION (v2) ---
    // Only print result if it's an expression that would leave a value on stack
    if (ast != nullptr && (dynamic_cast<Expression*>(ast) || dynamic_cast<AssignmentExpression*>(ast))) {
         std::cout << result << std::endl;
    } else if (dynamic_cast<PrintStatement*>(ast)) {
        // Print statements handle their own output within the VM, so no direct result here
        std::cout << "Output handled by PRINT instruction." << std::endl;
    }
    // For other statement types (like variable declarations without direct expression results),
    // we don't need to print a "No direct result" message.
    // --- END NEW IMPLEMENTATION (v2) ---
    std::cout << "------------" << std::endl;

    // Clean up AST
    delete ast;
}

int main(int argc, char* argv[]) {
    // --- NEW IMPLEMENTATION (v1) ---
    // Redirect std::cerr to std::cout for easier debugging in this environment
    std::cerr.rdbuf(std::cout.rdbuf());
    // --- END NEW IMPLEMENTATION (v1) ---
    std::cout << "Welcome to CoCompiler!" << std::endl;

    if (argc > 1) {
        // Process command-line arguments as source code files or direct strings
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            // Check if argument is a file path with .cocom extension
            if (arg.length() > 6 && arg.substr(arg.length() - 6) == ".cocom") {
                std::ifstream file(arg);
                if (file.is_open()) {
                    std::string file_content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                    process_source_code(file_content);
                    file.close();
                } else {
                    std::cerr << "Error: Could not open file '" << arg << "'" << std::endl;
                }
            } else if (arg.length() > 2 && arg.front() == '"' && arg.back() == '"') {
                // Treat as a direct source code string (remove quotes)
                process_source_code(arg.substr(1, arg.length() - 2));
            } else {
                std::cerr << "Error: Invalid argument. Expected a .cocom file path or a quoted string." << std::endl;
            }
        }
    } else {
        // Interactive mode if no command-line arguments
        std::cout << "Enter source code (type 'exit' to quit):" << std::endl;
        std::string line;
        while (true) {
            std::cout << "> ";
            std::getline(std::cin, line);
            if (line == "exit") {
                break;
            }
            if (!line.empty()) {
                process_source_code(line);
            }
        }
    }

    return 0;
}
