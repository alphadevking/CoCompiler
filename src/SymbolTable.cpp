#include "SymbolTable.h"
#include <iostream> // For debugging purposes, can be removed later

/**
 * @brief Constructs a new SymbolTable object.
 * Initializes with a global scope and resets the next available address.
 */
SymbolTable::SymbolTable() : next_address(0) {
    enterScope(); // Start with a global scope
}

/**
 * @brief Enters a new scope by pushing a new empty map onto the scopes stack.
 */
void SymbolTable::enterScope() {
    scopes.emplace_back();
}

/**
 * @brief Exits the current scope by popping the top map from the scopes stack.
 * Ensures there's always at least one scope (global scope).
 */
void SymbolTable::exitScope() {
    if (scopes.size() > 1) { // Don't pop the global scope
        scopes.pop_back();
    } else {
        // Optionally, handle error or log a warning if trying to exit global scope
        std::cerr << "Warning: Attempted to exit global scope." << std::endl;
    }
}

/**
 * @brief Adds a symbol to the current scope.
 * Assigns a unique address to the new symbol.
 * @param name The name of the symbol.
 * @param type The type of the symbol.
 * @return True if the symbol was added successfully, false if it already exists in the current scope.
 */
bool SymbolTable::addSymbol(const std::string& name, ASTNode::Type type) {
    if (scopes.empty()) {
        std::cerr << "Error: No active scope to add symbol to." << std::endl;
        return false;
    }
    // Check if the symbol already exists in the current scope
    if (scopes.back().count(name)) {
        std::cerr << "Error: Symbol '" << name << "' already exists in the current scope." << std::endl;
        return false;
    }
    // Assign the next available address and increment it
    scopes.back().emplace(name, Symbol(name, type, next_address++));
    return true;
}

/**
 * @brief Looks up a symbol in the current and enclosing scopes.
 * Searches from the innermost scope outwards to the global scope.
 * @param name The name of the symbol to look up.
 * @return A pointer to the Symbol if found, nullptr otherwise.
 */
Symbol* SymbolTable::lookupSymbol(const std::string& name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto& scope = *it;
        if (scope.count(name)) {
            return &scope.at(name);
        }
    }
    return nullptr; // Symbol not found in any scope
}
