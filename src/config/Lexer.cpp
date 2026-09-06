#include "Lexer.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>
#include <vector>

Lexer::Lexer(const std::string& filename): source(readFile(filename)), pos(0) {}

std::string Lexer::readFile(const std::string& filename) {
    std::ifstream file(filename.c_str());
    if (!file)
        throw std::runtime_error("Cannot open file");
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void Lexer::skipWhitespace() {
    while (pos < source.size() && std::isspace(static_cast<unsigned char>(source[pos])))
        pos++;
}

Token Lexer::nextToken() {
    std::string lexeme = "";
    skipWhitespace();

    while (pos < source.size() && source[pos] == '#')
    {
        skipComment();
        skipWhitespace();
    }

    if (pos >= source.size())
        return Token(Token::END, "");
    switch (source[pos])
    {
        case '{':
            ++pos;
            return Token(Token::LBRACE, "{");
            break;
        
        case '}':
            ++pos;
            return Token(Token::RBRACE, "}");
            break;
        
        case ';':
            ++pos;
            return Token(Token::SEMICOLON, ";");
            break;
        
        default:
            while (pos < source.size() && !std::isspace(static_cast<unsigned char>(source[pos])) && source[pos] != '{' && source[pos] != '}' && source[pos] != ';')
                lexeme.push_back(source[pos++]);
            return Token(Token::WORD, lexeme);
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    while (true) {
        Token token = nextToken();
        tokens.push_back(token);
        if (token.type == Token::END)
            break;
    }
    return tokens;
}

void Lexer::skipComment()
{
    while (pos < source.size() && source[pos] != '\n')
        ++pos;
}