#include "Lexer.h"
#include <cctype> // For isdigit, isalpha, isalnum
#include <iostream> // For error output

Lexer::Lexer(const std::string& source_code)
    : source(source_code), current_pos(0), current_line(1), current_column(1), errors({}) {}

// --- Helper Methods ---

char Lexer::peek() {
    if (isAtEnd()) return '\0';
    return source[current_pos];
}

char Lexer::advance() {
    if (isAtEnd()) return '\0';
    char c = source[current_pos];
    current_pos++;
    current_column++;
    return c;
}

bool Lexer::isAtEnd() {
    return current_pos >= source.length();
}

bool Lexer::isDigit(char c) {
    return std::isdigit(static_cast<unsigned char>(c));
}

bool Lexer::isAlpha(char c) {
    // Identifiers can start with a letter or an underscore
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool Lexer::isAlphaNumeric(char c) {
    // Identifiers can contain letters, digits, or underscores
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
        } else if (c == '\n') {
            advance();
            current_line++;
            current_column = 1; // Reset column on new line
        } else {
            break;
        }
    }
}

Token Lexer::number() {
    int start_pos = current_pos;
    int start_col = current_column;
    bool hasDecimal = false;

    while (isDigit(peek()) || peek() == '.') {
        if (peek() == '.') {
            if (hasDecimal) break; // Only one decimal point allowed
            hasDecimal = true;
        }
        advance();
    }

    std::string value = source.substr(start_pos, current_pos - start_pos);
    if (hasDecimal) {
        return Token(TokenType::FLOAT_LITERAL, value, current_line, start_col);
    } else {
        return Token(TokenType::INT_LITERAL, value, current_line, start_col);
    }
}

/**
 * @brief Lexes a string literal enclosed in double quotes.
 * Handles basic escape sequences (e.g., \").
 * @return A Token representing the string literal.
 */
Token Lexer::string_literal() {
    int start_pos = current_pos;
    int start_col = current_column;
    advance(); // Consume the opening '"'

    std::string value_builder;
    while (peek() != '"' && !isAtEnd()) {
        char c = advance();
        if (c == '\\') { // Handle escape sequences
            if (peek() == '"') {
                value_builder += '"';
                advance();
            } else if (peek() == '\\') {
                value_builder += '\\';
                advance();
            } else {
                // For simplicity, other escape sequences are not handled, just add the char
                value_builder += c;
            }
        } else {
            value_builder += c;
        }
    }

    if (isAtEnd()) {
        errors.push_back("Lexer Error: Unterminated string literal at L" + std::to_string(current_line) + ":C" + std::to_string(start_col));
        return Token(TokenType::EOF_TOKEN, "", current_line, start_col); // Return error token
    }

    advance(); // Consume the closing '"'
    return Token(TokenType::STRING_LITERAL, value_builder, current_line, start_col);
}

// --- Main Tokenization Logic ---

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (!isAtEnd()) {
        skipWhitespace();

        if (isAtEnd()) break;

        char c = peek();
        int token_start_col = current_column; // Store column *before* advancing

        if (isDigit(c)) {
            tokens.push_back(number());
            continue; // Go to next iteration after consuming the number
        }

        // Handle identifiers and keywords (like 'var', 'true', 'false', 'if', 'else', 'print')
        if (isAlpha(c)) {
            int identifier_start_pos = current_pos;
            int identifier_start_col = current_column;
            while (isAlphaNumeric(peek())) {
                advance();
            }
            std::string value = source.substr(identifier_start_pos, current_pos - identifier_start_pos);

            // Check for keywords
            if (value == "var") {
                tokens.push_back(Token(TokenType::VAR, value, current_line, identifier_start_col));
            } else if (value == "if") {
                tokens.push_back(Token(TokenType::IF, value, current_line, identifier_start_col));
            } else if (value == "else") {
                tokens.push_back(Token(TokenType::ELSE, value, current_line, identifier_start_col));
            } else if (value == "print") {
                tokens.push_back(Token(TokenType::PRINT, value, current_line, identifier_start_col));
            } else if (value == "true") { // New: true keyword
                tokens.push_back(Token(TokenType::TRUE, value, current_line, identifier_start_col));
            } else if (value == "false") { // New: false keyword
                tokens.push_back(Token(TokenType::FALSE, value, current_line, identifier_start_col));
            }
            else {
                tokens.push_back(Token(TokenType::IDENTIFIER, value, current_line, identifier_start_col));
            }
            continue; // Go to next iteration after consuming the identifier/keyword
        }

        switch (c) {
            case '+': tokens.push_back(Token(TokenType::PLUS, "+", current_line, token_start_col)); advance(); break;
            case '-': tokens.push_back(Token(TokenType::MINUS, "-", current_line, token_start_col)); advance(); break;
            case '*': tokens.push_back(Token(TokenType::STAR, "*", current_line, token_start_col)); advance(); break;
            case '/':
                advance(); // Consume first '/'
                if (peek() == '/') {
                    // It's a single-line comment, consume until newline or EOF
                    while (peek() != '\n' && !isAtEnd()) {
                        advance();
                    }
                    // Do not add a token for the comment
                } else {
                    // It's just a division operator
                    tokens.push_back(Token(TokenType::SLASH, "/", current_line, token_start_col));
                }
                break;
            case '(': tokens.push_back(Token(TokenType::LPAREN, "(", current_line, token_start_col)); advance(); break;
            case ')': tokens.push_back(Token(TokenType::RPAREN, ")", current_line, token_start_col)); advance(); break;
            case '{': tokens.push_back(Token(TokenType::LBRACE, "{", current_line, token_start_col)); advance(); break;
            case '}': tokens.push_back(Token(TokenType::RBRACE, "}", current_line, token_start_col)); advance(); break;
            case ';': tokens.push_back(Token(TokenType::SEMICOLON, ";", current_line, token_start_col)); advance(); break;
            case '"': tokens.push_back(string_literal()); break; // New: String literal
            case '=':
                advance(); // Consume '='
                if (peek() == '=') {
                    advance(); // Consume second '='
                    tokens.push_back(Token(TokenType::EQUAL_EQUAL, "==", current_line, token_start_col));
                } else {
                    tokens.push_back(Token(TokenType::ASSIGN, "=", current_line, token_start_col));
                }
                break;
            case '>':
                advance(); // Consume '>'
                if (peek() == '=') {
                    advance(); // Consume '='
                    tokens.push_back(Token(TokenType::GREATER_EQUAL, ">=", current_line, token_start_col));
                } else {
                    tokens.push_back(Token(TokenType::GREATER, ">", current_line, token_start_col));
                }
                break;
            case '<':
                advance(); // Consume '<'
                if (peek() == '=') {
                    advance(); // Consume '='
                    tokens.push_back(Token(TokenType::LESS_EQUAL, "<=", current_line, token_start_col));
                } else {
                    tokens.push_back(Token(TokenType::LESS, "<", current_line, token_start_col));
                }
                break;
            case '!':
                advance(); // Consume '!'
                if (peek() == '=') {
                    advance(); // Consume '='
                    tokens.push_back(Token(TokenType::BANG_EQUAL, "!=", current_line, token_start_col));
                } else {
                    tokens.push_back(Token(TokenType::BANG, "!", current_line, token_start_col)); // New: Handle standalone '!'
                }
                break;
            case '&':
                advance(); // Consume '&'
                if (peek() == '&') {
                    advance(); // Consume second '&'
                    tokens.push_back(Token(TokenType::AND, "&&", current_line, token_start_col));
                } else {
                    errors.push_back("Lexer Error: Unexpected character '&' at L" + std::to_string(current_line) + ":C" + std::to_string(current_column - 1));
                }
                break;
            case '|':
                advance(); // Consume '|'
                if (peek() == '|') {
                    advance(); // Consume second '|'
                    tokens.push_back(Token(TokenType::OR, "||", current_line, token_start_col));
                } else {
                    errors.push_back("Lexer Error: Unexpected character '|' at L" + std::to_string(current_line) + ":C" + std::to_string(current_column - 1));
                }
                break;
            default:
                errors.push_back("Lexer Error: Unexpected character '" + std::string(1, c) + "' at L" + std::to_string(current_line) + ":C" + std::to_string(current_column));
                advance(); // Advance to avoid infinite loop on bad char
                break;
        }
    }

    // Add EOF token at the end
    tokens.push_back(Token(TokenType::EOF_TOKEN, "", current_line, current_column));

    if (!errors.empty()) {
        std::cerr << "\n--- Lexer Errors ---" << std::endl;
        for (const auto& error : errors) {
            std::cerr << error << std::endl;
        }
        std::cerr << "--------------------" << std::endl;
    }

    return tokens;
}
