#ifndef PARSER_HPP
#define PARSER_HPP

#include "Token.hpp"
#include "Config.hpp"
#include "Server.hpp"
#include "Location.hpp"
#include <vector>
#include <string>
#include <utility>

class Parser
{
    private:
        std::vector<Token> tokens;
        size_t pos;

        Config parseConfig();
        Server parseServer();
        Location parseLocation();
        std::pair<std::string, std::vector<std::string>> parseDirective();
        const Token& consume();
        const Token& current() const;
        Token expect(Token::Type type);

    public:
        Parser(const std::vector<Token>& tokens);
        Config parse();
};

#endif