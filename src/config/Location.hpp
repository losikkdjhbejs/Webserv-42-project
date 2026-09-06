#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <string>
#include <map>
#include <vector>

class Location {
    public:
        std::string path;
        std::map<std::string, std::vector<std::string> > directives;
};

#endif