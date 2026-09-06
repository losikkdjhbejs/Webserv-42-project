#include "MimeTypes.hpp"

MimeTypes::MimeTypes()
{
    _types[".html"] = "text/html";
    _types[".htm"] = "text/html";
    _types[".css"] = "text/css";
    _types[".js"] = "application/javascript";
    _types[".json"] = "application/json";
    _types[".png"] = "image/png";
    _types[".jpg"] = "image/jpeg";
    _types[".jpeg"] = "image/jpeg";
    _types[".gif"] = "image/gif";
    _types[".svg"] = "image/svg+xml";
    _types[".ico"] = "image/x-icon";
    _types[".txt"] = "text/plain";
    _types[".pdf"] = "application/pdf";
    _types[".py"] = "text/x-python";
}

std::string MimeTypes::getType(const std::string& extension) const{
    std::map<std::string, std::string>::const_iterator it = _types.begin();

    while (it != _types.end())
    {
        if (it->first == extension)
            return it->second;
        it++;
    }
    return "application/octet-stream";
}
