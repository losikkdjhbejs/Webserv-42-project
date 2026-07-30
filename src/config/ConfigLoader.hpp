#ifndef CONFIGLOADER_HPP
#define CONFIGLOADER_HPP

#include <string>

#include "Config.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "Token.hpp"
#include "Location.hpp"

class ConfigLoader {
    private:
        std::string filename;
    public:
        ConfigLoader(std::string filename);
        Config load();
};

#endif