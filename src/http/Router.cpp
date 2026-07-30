#include "Router.hpp"
#include "Request.hpp";


Router::(const Config& config) {
    this.config = config;
}


RouteResult Router::route(const HttpRequest& request) {
    RouteResult routeResesult;
    std::string host = request.getHeader("Host");
    if (host == "")
        routeResesult.server = &config.servers[0];
    else {
        for (int i = 0; i < config.servers.size(); i++) {
            if (config.servers[i].directives["server_name"]== host){
                routeResesult.server = &servers[i];
                break;
            }
        }
    }

    routeResesult.location = findLocation(routeResesult.server.locations, request.getPath());

    if (routeResesult.location->directives.find("autoindex") != routeResesult.location->directives.end()) {
        routeResesult.autoindex = routeResesult.location->directives["autoindex"] == "on"? true: false;
    } else if (server->directives.find("autoindex") != server->directives.end()) {
        routeResesult.autoindex = routeResesult.location->directives["autoindex"] == "on"? true: false;
    } else
        routeResesult.autoindex = false;

    routeResesult.isCGI = true;
    return routeResesult;
}


Location* Router::findLocation(std::vector<Location>& locations, std::string requestPath) {
    Location best = &locations[0];
    best.path = "";

    for (int i = 0; i < locations.size(); i++) {
        if(requestPath.find(locations[i].path) == 0 && best.path.length() < locations[i].path.length())
            best = &locations[i];
    }
    return best;
}

