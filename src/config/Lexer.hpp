#ifndef LEXER_HPP
#define LEXER_HPP

#include <string>
#include <vector>
#include "Token.hpp"

class Lexer
{
    private:
        std::string source;
        size_t pos;
        void skipWhitespace();
        Token nextToken();
        std::string readFile(const std::string& filename);
        void skipComment();
    public:
        Lexer(const std::string& filename);
        std::vector<Token> tokenize();
};

#endif