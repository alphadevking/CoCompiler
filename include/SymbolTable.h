#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <string>
#include <unordered_map>
#include <vector>
#include "AST.h" // For ASTNode::Type

/**
 * @brief Represents a symbol in the symbol table.
 */
struct Symbol {
    std::string name; /**< The name of the symbol. */
    ASTNode::Type type; /**< The type of the symbol. */
    int address; /**< The memory address assigned to the symbol. */

    /**
     * @brief Constructs a new Symbol object.
     * @param name The name of the symbol.
     * @param type The type of the symbol.
     * @param address The memory address assigned to the symbol.
     */
    Symbol(const std::string& name, ASTNode::Type type, int address)
        : name(name), type(type), address(address) {}
};

/**
 * @brief Manages symbols and their scopes during compilation.
 */
class SymbolTable {
public:
    /**
     * @brief Constructs a new SymbolTable object.
     */
    SymbolTable();

    /**
     * @brief Enters a new scope.
     */
    void enterScope();

    /**
     * @brief Exits the current scope.
     */
    void exitScope();

    /**
     * @brief Adds a symbol to the current scope.
     * @param name The name of the symbol.
     * @param type The type of the symbol.
     * @return True if the symbol was added successfully, false if it already exists in the current scope.
     */
    bool addSymbol(const std::string& name, ASTNode::Type type);

    /**
     * @brief Looks up a symbol in the current and enclosing scopes.
     * @param name The name of the symbol to look up.
     * @return A pointer to the Symbol if found, nullptr otherwise.
     */
    Symbol* lookupSymbol(const std::string& name);

private:
    std::vector<std::unordered_map<std::string, Symbol>> scopes; /**< A stack of scopes, each mapping symbol names to Symbols. */
    int next_address; /**< The next available memory address for a new symbol. */
};

#endif // SYMBOL_TABLE_H
