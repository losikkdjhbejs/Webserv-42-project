#ifndef REQUESTHANDLER_HPP
#define REQUESTHANDLER_HPP


#include "HttpRequest.hpp"
#include "RouteResult.hpp"
#include "HttpResponse.hpp"

class RequestHandler
{
private:
    HttpResponse handleGet(const HttpRequest& request, const RouteResult& routeResult);
    HttpResponse handlePost(const HttpRequest& request, const RouteResult& routeResult);
    HttpResponse handleDelete(const HttpRequest& request, const RouteResult& routeResult);
    HttpResponse handleJsonPost(const HttpRequest &request, const RouteResult &routeResult);
    HttpResponse saveRequestBodyToFile(const HttpRequest &request, const RouteResult &routeResult);
    HttpResponse buildCGIResponse(const std::vector<char>& data);
    HttpResponse handleCGI(const HttpRequest& request, const RouteResult& routeResult);
    HttpResponse handleMultipartUpload(const HttpRequest &request, const RouteResult &routeResult);
    std::string getExtension(const std::string& path);
    HttpResponse serveFile(const std::string& filePath);
    HttpResponse handleDirectory(const HttpRequest& request, const RouteResult& routeResult);
    HttpResponse generateAutoIndex(const HttpRequest &request, const RouteResult &RouteResult);
    HttpResponse handleError(const HttpRequest& request, int status, const RouteResult &routeResult);
    bool valid_body_size(const HttpRequest &request, const RouteResult &routeResult);
public:
    RequestHandler();

    HttpResponse handle(const HttpRequest& request, const RouteResult& routeResult);
};

#endif