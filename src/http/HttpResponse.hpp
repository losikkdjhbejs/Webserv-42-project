class HttpResponse
{
public:
    HttpResponse();

    void setVersion(HttpRequest::Version version);

    void setStatus(int code);

    void setHeader(const std::string& key,
                   const std::string& value);

    bool hasHeader(const std::string& key) const;

    const std::string& getHeader(const std::string& key) const;

    void setBody(const std::vector<char>& body);

    const std::vector<char>& getBody() const;

    std::vector<char> serialize() const;

private:
    HttpRequest::Version version;

    int statusCode;
    std::string reasonPhrase;

    std::map<std::string,std::string> headers;

    std::vector<char> body;
};