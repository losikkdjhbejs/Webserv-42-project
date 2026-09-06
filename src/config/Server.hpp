#ifndef SERVER_HPP
#define SERVER_HPP

#include "Location.hpp"
#include <string>

class Server {
    public:
        std::map<std::string, std::vector<std::string> > directives;
        std::vector<Location> locations;
};


#endif