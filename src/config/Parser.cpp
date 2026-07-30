#include "Parser.hpp"
#include "Token.hpp"
#include <stdexcept>
#include <map>

Parser::Parser(const std::vector<Token>& tokens): tokens(tokens), pos(0)
{}

const Token& Parser::current() const
{
    return tokens[pos];
}

const Token& Parser::consume()
{
    return tokens[pos++];
}

Token Parser::expect(Token::Type type)
{
    if (current().type != type)
        throw std::runtime_error("unexpected token");
    return consume();
}

std::pair<std::string, std::vector<std::string> > Parser::parseDirective()
{
    Token title = expect(Token::WORD);

    std::vector<std::string> args;

    while (current().type == Token::WORD)
        args.push_back(consume().value);

    if (args.empty())
        throw std::runtime_error("directive requires arguments");

    expect(Token::SEMICOLON);

    return std::make_pair(title.value, args);
}

Location Parser::parseLocation()
{
    Location location;
    std::map<std::string, std::vector<std::string>> directives;

    Token locationBlock = expect(Token::WORD);

    if (locationBlock.value != "location")
        throw std::runtime_error("Unexpeted location block");
    Token pathToken = expect(Token::WORD);
    expect(Token::LBRACE);
    while (current().type != Token::RBRACE) {
        std::pair<std::string, std::vector<std::string> > d = parseDirective();             
        if (directives.count(d.first))
            throw std::runtime_error("duplicate directive");

        directives.insert(d);
    }
    expect(Token::RBRACE);
    location.path = pathToken.value;
    location.directives = directives;
    return location;
}

Server Parser::parseServer() {
    Server server;
    Token serverToken = expect(Token::WORD);
    if (serverToken.value != "server")
        throw std::runtime_error("unexpected token");
    expect(Token::LBRACE);
    while (current().type != Token::RBRACE)
    {
        if (current().type == Token::WORD && current().value == "location") {
            Location loc = parseLocation();
            for (size_t i = 0; i < server.locations.size(); i++)
            {
                if (server.locations[i].path == loc.path)
                    throw std::runtime_error("duplicate location");
            }
            server.locations.push_back(loc);
        }
        else {
            std::pair<std::string, std::vector<std::string>> d = parseDirective();
            if (server.directives.count(d.first))
                throw std::runtime_error("duplicate directive");

            server.directives.insert(d);
        }
    }
    expect(Token::RBRACE);
    if (server.directives.find("listen") == server.directives.end())
        throw std::runtime_error("missing listen");

    if (server.directives.find("host") == server.directives.end())
        throw std::runtime_error("missing host");

    if (server.directives.find("root") == server.directives.end())
        throw std::runtime_error("missing root");
    return server;
    }

Config Parser::parseConfig() {
    Config config;
    while (current().type != Token::END)
    {
        config.servers.push_back(parseServer());
    }
    return config;
}


Config Parser::parse() {
    return parseConfig();
}