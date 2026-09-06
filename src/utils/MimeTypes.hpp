#ifndef MIMETYPES_HPP
#define MIMETYPES_HPP

#include <string>
#include <map>

class MimeTypes
{
    private:
        std::map<std::string, std::string> _types;
    public:
        MimeTypes();
        std::string getType(const std::string& extension) const;
};

#endif