#include "HttpResponse.hpp"
#include "HttpRequest.hpp"

#include <sstream>
#include <stdexcept>

HttpResponse::HttpResponse() {
    version = HttpRequest::HTTP_1_1;
    statusCode = 200;
    reasonPhrase = "OK";
}



void HttpResponse::setVersion(HttpRequest::Version version) {
    this->version = version;
}

void HttpResponse::setStatus(int code)
{
    statusCode = code;

    switch (code)
    {
        case 200: reasonPhrase = "OK"; break;
        case 201: reasonPhrase = "Created"; break;
        case 204: reasonPhrase = "No Content"; break;
        case 301: reasonPhrase = "Moved Permanently"; break;
        case 302: reasonPhrase = "Found"; break;
        case 400: reasonPhrase = "Bad Request"; break;
        case 403: reasonPhrase = "Forbidden"; break;
        case 404: reasonPhrase = "Not Found"; break;
        case 405: reasonPhrase = "Method Not Allowed"; break;
        case 413: reasonPhrase = "Payload Too Large"; break;
        case 500: reasonPhrase = "Internal Server Error"; break;
        default:
            reasonPhrase = "Unknown Status";
    }
}
void HttpResponse::setHeader(const std::string& key, const std::string& value) {
    headers[key] = value;
}

bool HttpResponse::hasHeader(const std::string& key) const {
    return headers.find(key) != headers.end();
}

const std::string& HttpResponse::getHeader(const std::string& key) const {
    std::map<std::string,std::string>::const_iterator it = headers.find(key);
    if (it == headers.end())
        throw std::runtime_error("Header not found");

    return it->second;
}

void HttpResponse::setBody(const std::vector<char>& body) {
    this->body = body;
}

const std::vector<char>& HttpResponse::getBody() const {
    return body;
}

std::vector<char> HttpResponse::serialize() const {
    std::string text;
    std::vector<char> response;
    if (version == HttpRequest::HTTP_1_0){
        text = "HTTP/1.0";
        response.insert(response.end(), text.begin(), text.end());
    } else {
        text = "HTTP/1.1";
        response.insert(response.end(), text.begin(), text.end());
    }
    response.push_back(' ');
    std::stringstream ss;
    ss << statusCode;

    text = ss.str();
    response.insert(response.end(), text.begin(), text.end());
    response.push_back(' ');
    response.insert(response.end(), reasonPhrase.begin(), reasonPhrase.end());
    response.insert(response.end(), &"\r\n"[0], &"\r\n"[2]);
    std::map<std::string, std::string>::const_iterator it;
    for (it = headers.begin(); it != headers.end(); it++) {
        
        text = it->first;
        response.insert(response.end(), text.begin(), text.end());
        response.push_back(':');
        response.push_back(' ');
        text = it->second;
        response.insert(response.end(), text.begin(), text.end());
        response.insert(response.end(), &"\r\n"[0], &"\r\n"[2]);
    }
    if (!hasHeader("Content-Length")) {

        text = "Content-Length";
        response.insert(response.end(), text.begin(), text.end());
        response.push_back(':');
        response.push_back(' ');
        std::stringstream ss;
        ss << body.size();
        std::string length = ss.str();
        response.insert(response.end(), length.begin(), length.end());
        response.insert(response.end(), &"\r\n"[0], &"\r\n"[2]);
    }
    response.insert(response.end(), &"\r\n"[0], &"\r\n"[2]);
    response.insert(response.end(), body.begin(), body.end());
    return response;
}