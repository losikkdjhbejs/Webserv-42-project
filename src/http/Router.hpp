#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "../config/Config.hpp"
#include "../config/Location.hpp"
#include "RouteResult.hpp"
#include "HttpRequest.hpp"

class Router
{
    const Config& config;
    const Location* findLocation(const std::vector<Location>& locations, const std::string& requestPath);
    public:
        Router(const Config& config);
        RouteResult route(const HttpRequest& request, const Server &listeningServer);
};

#endif