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