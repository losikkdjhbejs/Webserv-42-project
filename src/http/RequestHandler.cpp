#include "RequestHandler.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "../utils/MimeTypes.hpp"
#include <vector>
#include <sys/stat.h>
#include <sys/wait.h>
#include <iostream>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <fstream>
#include <iterator>
#include <dirent.h>
#include <string>
#include <cstdlib>

RequestHandler::RequestHandler() {}

HttpResponse RequestHandler::handle(const HttpRequest &request, const RouteResult &routResult)
{
    if (request.getMethod() == HttpRequest::GET)
        return handleGet(request, routResult);
    else if (request.getMethod() == HttpRequest::POST)
        return handlePost(request, routResult);
    else if (request.getMethod() == HttpRequest::DELETE)
        return handleDelete(request, routResult);
    HttpResponse response = HttpResponse();
    response.setVersion(request.getVersion());
    response.setStatus(405);
    std::string body = "Method not supported.";
    response.setBody(std::vector<char>(body.begin(), body.end()));
    return response;
}

HttpResponse RequestHandler::handleGet(const HttpRequest &request, const RouteResult &routeResult)
{
    HttpResponse response;

    response.setVersion(request.getVersion());

    struct stat pathInfo;

    if (routeResult.location == NULL)
        std::cout << "location is NULL!" << std::endl;
    else
    {
        std::cout << "location ptr = " << routeResult.location << std::endl;
        std::cout << "location path = " << routeResult.location->path << std::endl;
    }

    if (routeResult.location != NULL)
    {
        std::map<std::string, std::vector<std::string> >::const_iterator it =
            routeResult.location->directives.find("return");

        if (it != routeResult.location->directives.end() &&
            !it->second.empty())
        {
            response.setStatus(302);
            response.setHeader("Location", it->second[0]);

            std::string body = "Redirecting to " + it->second[0];
            response.setBody(std::vector<char>(body.begin(), body.end()));

            return response;
        }
    }
    if (routeResult.isCGI)
        return handleCGI(request, routeResult);
    if (stat(routeResult.fileSystemPath.c_str(), &pathInfo) != 0)
        return handleError(request, 404, routeResult);
    if (S_ISREG(pathInfo.st_mode))
        return serveFile(routeResult.fileSystemPath);
    if (S_ISDIR(pathInfo.st_mode))
        return handleDirectory(request, routeResult);
    return handleError(request, 403, routeResult);
}

HttpResponse RequestHandler::serveFile(const std::string &fileSystemPath)
{
    HttpResponse response;
    std::ifstream file(fileSystemPath.c_str(), std::ios::binary);

    if (!file.is_open())
    {
        response.setStatus(403);
        std::string text = "Forbidden";
        response.setBody(std::vector<char>(text.begin(), text.end()));
        return response;
    }
    std::vector<char> body((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    MimeTypes mimeType;
    std::string ext = getExtension(fileSystemPath);
    response.setHeader("Content-Type", mimeType.getType(ext));

    response.setStatus(200);
    response.setBody(body);
    return response;
}

HttpResponse RequestHandler::handleDirectory(const HttpRequest &request, const RouteResult &routeResult)
{
    HttpResponse response;

    if (routeResult.location == NULL)
    {
        response.setStatus(500);
        std::cout << "location block is still NULL\n";
        std::string text = "Internal Server Error";
        response.setBody(std::vector<char>(text.begin(), text.end()));
        return response;
    }
    /*
     * 1. Try configured index files.
     */

    std::cout << "location ptr = " << routeResult.location << std::endl;
    std::cout << "location = " << routeResult.location->path << std::endl;

    std::map<std::string, std::vector<std::string> >::const_iterator it =
        routeResult.location->directives.find("index");

    if (it != routeResult.location->directives.end())
    {
        const std::vector<std::string> &indexFiles = it->second;

        for (size_t i = 0; i < indexFiles.size(); ++i)
        {
            std::string indexPath =
                routeResult.fileSystemPath + "/" + indexFiles[i];

            struct stat indexInfo;

            if (stat(indexPath.c_str(), &indexInfo) == 0 &&
                S_ISREG(indexInfo.st_mode))
            {
                return serveFile(indexPath);
            }
        }
    }

    if (routeResult.autoindex)
        return generateAutoIndex(request, routeResult);

    /*
     * 3. No index + autoindex disabled.
     */
    response.setStatus(403);

    std::string text = "Directory access forbidden";
    response.setBody(
        std::vector<char>(text.begin(), text.end()));

    return response;
}

HttpResponse RequestHandler::generateAutoIndex(const HttpRequest &request, const RouteResult &routeResult)
{

    HttpResponse response;

    DIR *dir = opendir(routeResult.fileSystemPath.c_str());

    if (dir == NULL)
    {
        response.setStatus(403);
        std::string text = "Forbidden";
        response.setBody(std::vector<char>(text.begin(), text.end()));
        return response;
    }

    std::string html;

    html += "<html>\n";
    html += "<head><title>Index of ";
    html += request.getPath();
    html += "</title></head>\n";

    html += "<body>\n";
    html += "<h1>Index of ";
    html += request.getPath();
    html += "</h1>\n";

    html += "<ul>\n";

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;

        if (name == "." || name == "..")
            continue;

        html += "<li><a href=\"";
        html += name;
        html += "\">";
        html += name;
        html += "</a></li>\n";
    }

    html += "</ul>\n";
    html += "</body>\n";
    html += "</html>\n";

    closedir(dir);

    response.setStatus(200);
    response.setHeader("Content-Type", "text/html");

    response.setBody(
        std::vector<char>(html.begin(), html.end()));

    return response;
}

HttpResponse RequestHandler::handlePost(const HttpRequest &request, const RouteResult &routeResult) {
    HttpResponse response;
    response.setVersion(request.getVersion());

    // CGI POST
    if (routeResult.isCGI)
        return handleCGI(request, routeResult);

    struct stat pathInfo;

    bool exists =
        (stat(routeResult.fileSystemPath.c_str(), &pathInfo) == 0);

    // Existing resource must be a regular file
    if (exists && !S_ISREG(pathInfo.st_mode))
    {
        return handleError(request, 403, routeResult);
    }

    // Create / overwrite file
    std::ofstream file(
        routeResult.fileSystemPath.c_str(),
        std::ios::binary);

    if (!file.is_open())
    {
        return handleError(request, 500, routeResult);
    }

    const std::vector<char>& body = request.getBody();

    if (!body.empty())
    {
        file.write(&body[0], body.size());

        if (!file)
        {
            file.close();
            return handleError(request, 500, routeResult);
        }
    }

    file.close();

    response.setStatus(201);

    std::string text = "Created";

    response.setBody(
        std::vector<char>(
            text.begin(),
            text.end()));

    return response;
}

HttpResponse RequestHandler::handleDelete(const HttpRequest &request, const RouteResult &routeResult)
{
    HttpResponse response;
    response.setVersion(request.getVersion());

    struct stat pathInfo;
    // 1. Check if resource exists
    if (stat(routeResult.fileSystemPath.c_str(), &pathInfo) != 0)
    {
        return handleError(request, 404, routeResult);
    }

    // 2. Only allow regular files
    if (!S_ISREG(pathInfo.st_mode))
    {
        response.setStatus(403);

        std::string body = "Forbidden";
        response.setBody(std::vector<char>(body.begin(), body.end()));

        return response;
    }

    // 3. Delete file
    if (unlink(routeResult.fileSystemPath.c_str()) != 0)
    {
        response.setStatus(403);

        std::string body = "Forbidden";
        response.setBody(
            std::vector<char>(body.begin(), body.end()));

        return response;
    }

    // 4. Successfully deleted
    response.setStatus(204);

    return response;
}

HttpResponse RequestHandler::buildCGIResponse(const std::vector<char> &data)
{
    HttpResponse response;

    std::string output(data.begin(), data.end());

    size_t headerEnd = output.find("\r\n\r\n");
    size_t separatorLength = 4;

    if (headerEnd == std::string::npos)
    {
        headerEnd = output.find("\n\n");
        separatorLength = 2;
    }

    // No CGI headers -> treat everything as body
    if (headerEnd == std::string::npos)
    {
        response.setStatus(200);
        response.setBody(data);
        return response;
    }

    std::string headers = output.substr(0, headerEnd);
    std::string body = output.substr(headerEnd + separatorLength);

    response.setStatus(200);

    size_t start = 0;

    while (start < headers.size())
    {
        size_t end = headers.find('\n', start);

        if (end == std::string::npos)
            end = headers.size();

        std::string line = headers.substr(start, end - start);

        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        size_t colon = line.find(':');

        if (colon != std::string::npos)
        {
            std::string name = line.substr(0, colon);
            std::string value = line.substr(colon + 1);

            while (!value.empty() && value[0] == ' ')
                value.erase(0, 1);

            response.setHeader(name, value);
        }

        start = end + 1;
    }

    response.setBody(std::vector<char>(body.begin(), body.end()));

    return response;
}

HttpResponse RequestHandler::handleCGI(const HttpRequest &request, const RouteResult &routeResult)
{
    HttpResponse response;
    response.setVersion(request.getVersion());

    if (access(routeResult.fileSystemPath.c_str(), F_OK) != 0)
        return handleError(request, 404, routeResult);
    if (access(routeResult.fileSystemPath.c_str(), X_OK) != 0)
        return handleError(request, 403, routeResult);
    int inputPipe[2];
    int outputPipe[2];

    if (pipe(inputPipe) == -1)
        return handleError(request, 500, routeResult);

    if (pipe(outputPipe) == -1)
    {
        close(inputPipe[0]);
        close(inputPipe[1]);
        return handleError(request, 500, routeResult);
    }

    pid_t pid = fork();

    if (pid < 0)
    {
        close(inputPipe[0]);
        close(inputPipe[1]);
        close(outputPipe[0]);
        close(outputPipe[1]);
        return handleError(request, 500, routeResult);
    }
    if (pid > 0)
    {
        close(inputPipe[0]);
        close(outputPipe[1]);

        const std::vector<char> &requestBody = request.getBody();
        size_t totalWritten = 0;
        while (totalWritten < requestBody.size())
        {
            ssize_t bytesWritten = write(inputPipe[1], &requestBody[totalWritten], requestBody.size() - totalWritten);
            if (bytesWritten <= 0)
            {
                close(inputPipe[1]);
                close(outputPipe[0]);
                waitpid(pid, NULL, 0);
                return handleError(request, 500, routeResult);
            }
            totalWritten += bytesWritten;
        }
        close(inputPipe[1]);
        char buffer[1024];
        std::vector<char> responseData;

        ssize_t bytesRead;
        while ((bytesRead = read(outputPipe[0], buffer, sizeof(buffer))) > 0)
            responseData.insert(responseData.end(), buffer, buffer + bytesRead);
        close(outputPipe[0]);
        int status = 0;
        if (waitpid(pid, &status, 0) == -1)
            return handleError(request, 500, routeResult);
        if (WIFEXITED(status))
        {
            int exitCode = WEXITSTATUS(status);
            if (exitCode != 0)
                return handleError(request, 500, routeResult);
        }
        else if (WIFSIGNALED(status))
            return handleError(request, 500, routeResult);
        return buildCGIResponse(responseData);
    }

    close(inputPipe[1]);
    close(outputPipe[0]);

    if (dup2(inputPipe[0], STDIN_FILENO) == -1)
        _exit(1);

    if (dup2(outputPipe[1], STDOUT_FILENO) == -1)
        _exit(1);

    close(inputPipe[0]);
    close(outputPipe[1]);

    char *args[] = {
        const_cast<char *>(routeResult.cgiExecutable.c_str()),
        const_cast<char *>(routeResult.fileSystemPath.c_str()),
        NULL};

    execv(args[0], args);

    _exit(1);
}

std::string RequestHandler::getExtension(const std::string &path)
{
    size_t dot = path.find_last_of(".");
    if (dot == std::string::npos)
        return "";
    return path.substr(dot);
}

HttpResponse RequestHandler::handleError(const HttpRequest &request, int status, const RouteResult &routeResult)
{
    HttpResponse response;
    response.setVersion(request.getVersion());
    if (routeResult.server != NULL)
    {
        std::map<std::string, std::vector<std::string> >::const_iterator it = routeResult.server->directives.find("error_page");
        if (it != routeResult.server->directives.end())
        {
            const std::vector<std::string> &values = it->second;
            // Search for: 404 /errors/404.html
            for (size_t i = 0; i + 1 < values.size(); i += 2)
            {
                if (std::atoi(values[i].c_str()) == status)
                {
                    std::map<std::string, std::vector<std::string> >::const_iterator rootIt = routeResult.server->directives.find("root");
                    if (rootIt != routeResult.server->directives.end() && !rootIt->second.empty())
                    {
                        std::string path = routeResult.server->directives.find("root")->second[0] + values[i + 1];

                        struct stat info;
                        if (stat(path.c_str(), &info) == 0 && S_ISREG(info.st_mode))
                        {
                            response.setStatus(status);
                            HttpResponse errorResponse = serveFile(path);
                            errorResponse.setStatus(status);
                            return errorResponse;
                        }
                    }
                }
            }
        }
    }

    response.setStatus(status);

    std::string text = "Error";
    response.setBody(std::vector<char>(text.begin(), text.end()));

    return response;
}
