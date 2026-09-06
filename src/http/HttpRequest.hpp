#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <vector>
#include <map>



class HttpRequest
{
public:
    enum Method
    {
        GET,
        POST,
        DELETE,
        UNKNOWN
    };

    enum Version
    {
        HTTP_1_0,
        HTTP_1_1
    };
private:
    enum State
    {
        REQUEST_LINE,
        HEADERS,
        BODY,
        COMPLETE,
        ERROR
    };


    Method method;
    Version version;
    std::string target;
    std::string path;
    
    std::map<std::string,std::string> queryParams;

    std::map<std::string,std::string> headers;

    std::vector<char> body;
    std::vector<char> buffer;

    State state;

    bool chunked;
    size_t contentLength;

    size_t chunkSize;
    bool readingChunkSize;
    
    void parse();
    bool parseRequestLine();
    bool parseHeaders();
    bool parseBody();
    Method parseMethod(const std::string& method);
    Version parseVersion(const std::string& version);
    void trim(std::string &s);
    bool extractLine(std::string& line);
    bool hexToInt(const std::string& txt, size_t& res);
public:
    HttpRequest();

    void appendData(const char* bytes,  size_t n);

    bool isComplete() const;
    bool hasError() const;

    void reset();

    Method getMethod() const;
    Version getVersion() const;

    const std::string& getTarget() const;
    const std::string& getPath() const;
    const std::map<std::string,std::string>& getQuery() const;

    const std::vector<char>& getBody() const;

    bool hasHeader(const std::string& key) const;
    const std::string& getHeader(const std::string& key) const;

    const std::map<std::string,std::string>& getHeaders() const;
    
    bool keepAlive() const;
};

#endif