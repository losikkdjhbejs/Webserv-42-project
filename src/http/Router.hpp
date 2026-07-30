class Router
{
    const Config& config;
    Location findLocation(std::string requestPath);
    public:
        Router(const Config& config);
        RouteResult route(const HttpRequest& request);
};
