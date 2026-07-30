class RouteResult
{
    public:
        const Server* server;
        const Location* location;

        std::string filesystemPath;

        bool isCGI;
        bool autoindex;

        RouteResult();
};