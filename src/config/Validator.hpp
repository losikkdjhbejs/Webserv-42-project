#ifndef VALIDATOR_HPP
#define VALIDATOR_HPP

#include "Config.hpp"

class Validator
{
    private:
        void validateServer(const Server& server);
        void validateLocation(const Location& location);
        void validateListen(const std::vector<std::string>& args);
        void validateRoot(const std::vector<std::string>& args);
        void validateIndex(const std::vector<std::string>& args);
        void validateMethods(const std::vector<std::string>& args);
        void validateAutoIndex(const std::vector<std::string>& args);
        void validateReturn(const std::vector<std::string>& args);
        void validateHost(const std::vector<std::string>& args);
        void validateServerName(const std::vector<std::string>& args);
        void validateClientMaxBodySize(const std::vector<std::string>& args);
        void validateErrorPage(const std::vector<std::string>& args);
        void validateCgiExt(const std::vector<std::string>& args);
        void validateCgiPath(const std::vector<std::string>& args);
    public:
        void validate(const Config& config);
};

#endif