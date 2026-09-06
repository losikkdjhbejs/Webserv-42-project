#ifndef ROUTERESULT_HPP
#define ROUTERESULT_HPP

#include "../config/Server.hpp"
class RouteResult
{
    public:
        const Server* server;
        const Location* location;

        std::string fileSystemPath;

        bool isCGI;
        bool autoindex;
        
        std::string cgiExecutable;
        RouteResult();
};

#endif