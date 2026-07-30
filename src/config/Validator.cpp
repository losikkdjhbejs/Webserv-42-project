#include "Validator.hpp"
#include <cstdlib>
#include <stdexcept>

void Validator::validate(const Config& config)
{
    for (size_t i = 0; i < config.servers.size(); i++)
        validateServer(config.servers[i]);
}

void Validator::validateServer(const Server& server)
{
    for (std::map<std::string,
         std::vector<std::string> >::const_iterator it =
         server.directives.begin();
         it != server.directives.end();
         ++it)
    
    {
        if (it->first == "listen")
            validateListen(it->second);
        else if (it->first == "host")
            validateHost(it->second);
        else if (it->first == "server_name")
            validateServerName(it->second);
        else if (it->first == "root")
            validateRoot(it->second);
        else if (it->first == "index")
            validateIndex(it->second);
        else if (it->first == "client_max_body_size")
            validateClientMaxBodySize(it->second);
        else if (it->first == "autoindex")
            validateAutoIndex(it->second);
        else if (it->first == "error_page")
            validateErrorPage(it->second);
        else if (it->first == "allow_methods")
            validateMethods(it->second);
        else if (it->first == "cgi_ext")
            validateCgiExt(it->second);
        else if (it->first == "cgi_path")
            validateCgiPath(it->second);
        else if (it->first == "return")
            validateReturn(it->second);
        else
            throw std::runtime_error("Unknown directive: " + it->first);
    }

    for (size_t i = 0; i < server.locations.size(); i++)
        validateLocation(server.locations[i]);
}

void Validator::validateLocation(const Location& location)
{
    for (std::map<std::string,
         std::vector<std::string> >::const_iterator it =
         location.directives.begin();
         it != location.directives.end();
         ++it)
    {
        if (it->first == "root")
            validateRoot(it->second);
        else if (it->first == "index")
            validateIndex(it->second);
        else if (it->first == "allow_methods")
            validateMethods(it->second);
        else if (it->first == "client_max_body_size")
            validateClientMaxBodySize(it->second);
        else if (it->first == "autoindex")
            validateAutoIndex(it->second);
        else if (it->first == "cgi_ext")
            validateCgiExt(it->second);
        else if (it->first == "cgi_path")
            validateCgiPath(it->second);
        else if (it->first == "return")
            validateReturn(it->second);
        else
            throw std::runtime_error("Unknown directive: " + it->first);
    }
}

void Validator::validateListen(const std::vector<std::string>& args)
{
    if (args.size() != 1)
        throw std::runtime_error("listen takes one argument");

    char* end;

    long port = std::strtol(args[0].c_str(), &end, 10);

    if (*end != '\0')
        throw std::runtime_error("invalid port");

    if (port < 1 || port > 65535)
        throw std::runtime_error("invalid port");
}

void Validator::validateRoot(const std::vector<std::string>& args)
{
    if (args.size() != 1)
        throw std::runtime_error("root takes one argument");
}

void Validator::validateMethods(const std::vector<std::string>& args)
{
    if (args.empty())
        throw std::runtime_error("allow_methods is empty");

    for (size_t i = 0; i < args.size(); i++)
    {
        if (args[i] != "GET" &&
            args[i] != "POST" &&
            args[i] != "DELETE")
            throw std::runtime_error("Invalid HTTP method");
    }
}

void Validator::validateAutoIndex(const std::vector<std::string>& args)
{
    if (args.size() != 1)
        throw std::runtime_error("autoindex takes one argument");

    if (args[0] != "on" &&
        args[0] != "off")
        throw std::runtime_error("autoindex must be on/off");
}


void Validator::validateHost(const std::vector<std::string>& args)
{
    if (args.size() != 1)
        throw std::runtime_error("host takes one argument");
}

void Validator::validateServerName(const std::vector<std::string>& args)
{
    if (args.size() != 1)
        throw std::runtime_error("server_name takes one argument");
}


void Validator::validateIndex(const std::vector<std::string>& args)
{
    if (args.size() != 1)
        throw std::runtime_error("index takes one argument");
}

void Validator::validateClientMaxBodySize(const std::vector<std::string>& args)
{
    if (args.size() != 1)
        throw std::runtime_error("client_max_body_size takes one argument");

        char *end;
    long value = std::strtol(args[0].c_str(), &end, 10);

    if (*end != '\0')
        throw std::runtime_error("invalid client_max_body_size");

    if (value <= 0)
        throw std::runtime_error("invalid client_max_body_size");
}

void Validator::validateErrorPage(const std::vector<std::string>& args)
{
    if (args.size() != 2)
        throw std::runtime_error("error_page takes two arguments");

    char *end;
    long code = std::strtol(args[0].c_str(), &end, 10);

    if (*end != '\0')
        throw std::runtime_error("invalid error code");

    if (code < 100 || code > 599)
        throw std::runtime_error("invalid error code");
}

void Validator::validateCgiExt(const std::vector<std::string>& args)
{
    if (args.empty())
        throw std::runtime_error("cgi_ext requires at least one extension");
}

void Validator::validateCgiPath(const std::vector<std::string>& args)
{
    if (args.empty())
        throw std::runtime_error("cgi_path requires at least one executable");
}

void Validator::validateReturn(const std::vector<std::string>& args)
{
    if (args.size() != 1)
        throw std::runtime_error("return takes one argument");
}