#include <map>
class Server {
    public:
        std::map<std::string, std::vector<std::string> > directives;
        const std::vector<Location>& locations;
};