#include "ConfigLoader.hpp"
#include <vector>
#include "Validator.hpp"

ConfigLoader::ConfigLoader(std::string filename): filename(filename){}


Config ConfigLoader::load()
{
    std::vector<Token> tokens = Lexer(filename).tokenize();

    Config config = Parser(tokens).parse();

    // Validator validator;
    // validator.validate(config);

    return config;
}