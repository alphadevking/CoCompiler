#ifndef VM_H
#define VM_H

#include <vector>
#include "../include/Bytecode.h"

class VM {
private:
    std::vector<Bytecode> bytecode;
    std::vector<double> stack; // Use double to store both ints and floats
    std::vector<double> memory; // New: For variable storage
    std::vector<std::string> string_literals; // New: To store string literals
    int pc; // Program counter

public:
    VM();
    double run(const std::vector<Bytecode>& bytecode, const std::vector<std::string>& string_literals);
};

#endif // VM_H
