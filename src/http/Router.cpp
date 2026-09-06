#include "Router.hpp"
#include "HttpRequest.hpp"
#include <stdexcept>
#include <iostream>

Router::Router(const Config &config) : config(config)
{
}

RouteResult Router::route(const HttpRequest &request)
{
    if (config.servers.empty())
        throw std::runtime_error("No server configured");
    RouteResult routeResult;
    std::string host;

    if (request.hasHeader("Host"))
        host = request.getHeader("Host");
    size_t pos = host.find(':');
    if (pos != std::string::npos)
        host = host.substr(0, pos);

    if (host == "")
        routeResult.server = &config.servers[0];
    else
    {
        for (size_t i = 0; i < config.servers.size(); i++)
        {

            std::map<std::string, std::vector<std::string> >::const_iterator it = config.servers[i].directives.find("server_name");

            if (it != config.servers[i].directives.end())
            {
                const std::vector<std::string> &names = it->second;

                for (size_t j = 0; j < names.size(); ++j)
                {
                    if (names[j] == host)
                    {
                        routeResult.server = &config.servers[i];
                        break;
                    }
                }
            }
            if (routeResult.server != NULL)
                break;
        }
    }

    if (routeResult.server == NULL)
        routeResult.server = &config.servers[0];

    routeResult.location = findLocation(routeResult.server->locations, request.getPath());

    routeResult.isCGI = false;

    if (routeResult.location != NULL)
    {
        std::map<std::string, std::vector<std::string> >::const_iterator it = routeResult.location->directives.find("cgi_ext");

        std::map<std::string, std::vector<std::string> >::const_iterator pathIt = routeResult.location->directives.find("cgi_path");

        if (it != routeResult.location->directives.end())
        {
            const std::vector<std::string> &extensions = it->second;

            const std::vector<std::string> &cgiPaths = pathIt->second;

            std::string path = request.getPath();

            for (size_t i = 0; i < extensions.size(); ++i)
            {
                const std::string &ext = extensions[i];

                if (path.length() >= ext.length() && path.compare(path.length() - ext.length(), ext.length(), ext) == 0)
                {
                    routeResult.isCGI = true;
                    routeResult.cgiExecutable = cgiPaths[i];
                    break;
                }
            }
        }
    }

    if (routeResult.location != NULL &&
        routeResult.location->directives.find("autoindex") !=
            routeResult.location->directives.end())
    {
        routeResult.autoindex =
            routeResult.location->directives.find("autoindex")->second[0] == "on";
    }
    else if (routeResult.server->directives.find("autoindex") !=
             routeResult.server->directives.end())
    {
        routeResult.autoindex =
            routeResult.server->directives.find("autoindex")->second[0] == "on";
    }
    else
    {
        routeResult.autoindex = false;
    }

    if (routeResult.location != NULL &&
        routeResult.location->directives.find("root") !=
            routeResult.location->directives.end())
    {
        routeResult.fileSystemPath =
            routeResult.location->directives.find("root")->second[0] + request.getPath();
    }
    else if (routeResult.server->directives.find("root") !=
             routeResult.server->directives.end())
    {
        routeResult.fileSystemPath =
            routeResult.server->directives.find("root")->second[0] + request.getPath();
    }
    else
    {
        routeResult.fileSystemPath = request.getPath();
    }

    return routeResult;
}

// const Location* Router::findLocation(const std::vector<Location>& locations, const std::string& requestPath) {
//     const Location* best = NULL;

//     std::cout << "remove me: locations size : " << locations.size() << std::endl;
//     std::cout << "remove me: request path : " << requestPath << std::endl;

//     for (size_t i = 0; i < locations.size(); ++i)
//     {
//         std::cout << "remove me: locations path : " << locations[i].path << std::endl;

//         if (requestPath == locations[i].path || (requestPath.find(locations[i].path) == 0 && requestPath[locations[i].path.length()] == '/'))
//         {
//             if (best == NULL || locations[i].path.length() > best->path.length())
//             {
//                 best = &locations[i];
//             }
//         }
//     }

//     std::cout << "Remove me: founded location path : " << best->path << std::endl;

//     return best;
// }

const Location *Router::findLocation(const std::vector<Location> &locations, const std::string &requestPath)
{
    const Location *best = NULL;

    std::cout << "remove me: locations size : " << locations.size() << std::endl;
    std::cout << "remove me: request path : " << requestPath << std::endl;

    for (size_t i = 0; i < locations.size(); ++i)
    {
        const std::string &locationPath = locations[i].path;

        std::cout << "remove me: locations path : " << locationPath << std::endl;

        bool match = false;

        /*
         * location /
         * matches every URI beginning with /
         */
        if (locationPath == "/")
        {
            match = !requestPath.empty() && requestPath[0] == '/';
        }
        /*
         * Exact match:
         * /redirect matches /redirect
         */
        else if (requestPath == locationPath)
        {
            match = true;
        }
        /*
         * Prefix match:
         * /cgi-bin matches /cgi-bin/test.py
         *
         * but /cgi-bin does NOT match /cgi-binary
         */
        else if (requestPath.find(locationPath) == 0 &&
                 requestPath.length() > locationPath.length() &&
                 requestPath[locationPath.length()] == '/')
        {
            match = true;
        }

        if (match)
        {
            if (best == NULL ||
                locationPath.length() > best->path.length())
            {
                best = &locations[i];
            }
        }
    }

    if (best != NULL)
    {
        std::cout << "Remove me: founded location path : "
                  << best->path << std::endl;
    }
    else
    {
        std::cout << "Remove me: no location matched" << std::endl;
    }

    return best;
}
