#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <string>

struct Token
{
        enum Type {
            WORD,
            LBRACE,
            RBRACE,
            SEMICOLON,
            END
        };
        Type type;
        std::string value;
        Token(Type type, const std::string& value):type(type), value(value) {}
};

#endif