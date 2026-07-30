#include  "HttpRequest.hpp"
#include <sstream>
#include <cstdlib>
#include <cctype>
#include  <vector>
#include <cerrno>


HttpRequest::HttpRequest()
: method(UNKNOWN),
  version(HTTP_1_1),
  state(REQUEST_LINE),
  chunked(false),
  contentLength(0),
  chunkSize(0),
  readingChunkSize(true)
{
}

void HttpRequest::parse() {
    while (true)
    {
        switch (state)
        {
            case REQUEST_LINE:
                if (!parseRequestLine())
                    return;
                break;
            case HEADERS:
                if (!parseHeaders())
                    return;
                break;
            case BODY:
                if (!parseBody())
                    return;
                break;
            case COMPLETE:
        case ERROR:
            return;
            default:
                break;
        }
    }
}

HttpRequest::Method HttpRequest::parseMethod(const std::string& text) {
    if (text == "POST")
        return POST;
    else if (text == "GET")
        return GET;
    else if (text == "DELETE")
        return DELETE;
    state = ERROR;
    return UNKNOWN;
}


HttpRequest::Version HttpRequest::parseVersion(const std::string& text) {
    if (text == "HTTP/1.0")
        return HTTP_1_0;
    else if(text == "HTTP/1.1")
        return HTTP_1_1;
    state = ERROR;
    return HTTP_1_1;
}

bool HttpRequest::parseRequestLine() {
    std::string line;
    if (!extractLine(line))
        return false;
    std::vector<std::string> parts;
    std::stringstream ss(line);
    std::string token;
    while (getline(ss, token, ' ')) {
        parts.push_back(token);
    }
    if (parts.size() != 3) {
        state = ERROR;
        return false;
    }
    method = parseMethod(parts[0]);
    if (state == ERROR)
        return false;

    target = parts[1];
    if (target.empty() || target[0] != '/')
    {
        state = ERROR;
        return false;
    }

    std::string::size_type pos = target.find('?');

    if (pos == std::string::npos)
    {
        path = target;
    }
    else
    {
        path = target.substr(0, pos);

        std::string query = target.substr(pos + 1);

        std::stringstream ss(query);
        std::string pair;

        while (std::getline(ss, pair, '&'))
        {
            std::string::size_type eq = pair.find('=');

            if (eq == std::string::npos)
            {
                queryParams[pair] = "";
            }
            else
            {
                queryParams[pair.substr(0, eq)] = pair.substr(eq + 1);
            }
        }
    }

    version = parseVersion(parts[2]);
    if (state == ERROR)
        return false;
    state = HEADERS;
    return true;
}

void HttpRequest::trim(std::string &s) {
    size_t start = 0;
    while (start < s.size() &&
           std::isspace(static_cast<unsigned char>(s[start])))
        ++start;

    size_t end = s.size();
    while (end > start &&
           std::isspace(static_cast<unsigned char>(s[end - 1])))
        --end;

    s = s.substr(start, end - start);
}

bool HttpRequest::parseHeaders() {
    std::string line;

    while (extractLine(line)) {

        if (line.empty())
        {
            if (version == HTTP_1_1 &&
                headers.find("host") == headers.end())
            {
                state = ERROR;
                return false;
            }

            if (chunked || contentLength > 0)
                state = BODY;
            else
                state = COMPLETE;

            return true;
        }

        size_t pos = line.find(':');
        if (pos == std::string::npos){
            state = ERROR;
            return false;
        }

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        trim(key);
        trim(value);
        
        for (size_t i = 0; i < key.size(); ++i)
            key[i] = std::tolower(static_cast<unsigned char>(key[i]));

        if (key == "content-length") {
            if (headers.find("content-length") != headers.end()) {
                state = ERROR;
                return false;
            }

            char *end;
            errno = 0;
            long len = std::strtol(value.c_str(), &end, 10);

            if (errno != 0 || *end != '\0' || len < 0) {
                state = ERROR;
                return false;
            }

            contentLength = static_cast<size_t>(len);
        }
        
        if (key == "connection" || key == "transfer-encoding") {
            for (size_t i = 0; i < value.size(); i++)
                value[i] = std::tolower(
                    static_cast<unsigned char>(value[i]));
        }

        if (key == "transfer-encoding")
        {
            if (headers.find("transfer-encoding") != headers.end())
            {
                state = ERROR;
                return false;
            }
            if (value != "chunked")
            {
                state = ERROR;
                return false;
            }
            chunked = true;
        }
        headers[key] = value;
    }
    
    return false;
}

bool HttpRequest::hexToInt(const std::string&  text, size_t& result) {
    if (text.empty())
        return false;
    result = 0;
    for (std::string::size_type i = 0; i < text.size(); i++) {
        int digit = 0;
        if (text[i] >= '0' && text[i] <= '9')
            digit += text[i] - '0';
        else if (text[i] >= 'A' && text[i] <= 'F')
            digit += text[i] - 'A'  + 10;
        else if (text[i] >= 'a' && text[i] <= 'f')
            digit += text[i] - 'a' + 10 ;
        else
            return false;
        result = result * 16 + digit;
    }
    return true;
}

bool HttpRequest::parseBody() {
    
    if (chunked && headers.find("content-length") != headers.end()) {
        state = ERROR;
        return false;
    }
    if (chunked) {
        std::string chunkData;
        while (!readingChunkSize || extractLine(chunkData)) {
            if (readingChunkSize) {
                std::string::size_type semi = chunkData.find(';');
                if (semi != std::string::npos)
                    chunkData.erase(semi);
                if (!hexToInt(chunkData, chunkSize))
                {
                    state = ERROR;
                    return false;
                }
                readingChunkSize = false;
            }
            if (chunkSize == 0) {
                if (buffer.size() >=2 && buffer[0] == '\r' && buffer[1] == '\n') {
                    buffer.erase(buffer.begin(), buffer.begin() + 2);
                    state = COMPLETE;
                    return true;
                }
                return false;
            }
            if (buffer.size() < chunkSize + 2)
                return false;
            if (buffer[chunkSize] != '\r' || buffer[chunkSize + 1] != '\n')
            {
                state = ERROR;
                return false;
            }
            body.insert(body.end(), buffer.begin(), buffer.begin() + chunkSize);
            buffer.erase(buffer.begin(), buffer.begin() + chunkSize + 2);
            chunkData.clear();
            readingChunkSize = true;
        } 
        return false;
        
    } else if (contentLength > 0) {
        if (buffer.size() < contentLength)
            return false;
        body.insert(body.end(), buffer.begin(), buffer.begin() + contentLength);
        buffer.erase(buffer.begin(), buffer.begin() + contentLength);
        state = COMPLETE;
        return true;
    } else {
        state = COMPLETE;
        return true;
    }
}

bool HttpRequest::extractLine(std::string& line)
{
    line.clear();

    for (size_t i = 0; i + 1 < buffer.size(); i++)
    {
        if (buffer[i] == '\r' && buffer[i + 1] == '\n')
        {
            line.assign(buffer.begin(), buffer.begin() + i);

            buffer.erase(buffer.begin(), buffer.begin() + i + 2);

            return true;
        }
    }

    return false;
}


void HttpRequest::appendData(const char* bytes,  size_t n) {
    buffer.insert(buffer.end(), bytes, bytes + n);
    parse();
}

bool HttpRequest::isComplete() const {
    return state == COMPLETE;
}

bool HttpRequest::hasError() const {
    return state == ERROR;
}

void HttpRequest::reset() {
    path.clear();
    target.clear();
    queryParams.clear();
    headers.clear();
    body.clear();
    buffer.clear();
    method = UNKNOWN;
    version = HTTP_1_1;
    state = REQUEST_LINE;
    chunked = false;
    contentLength = 0;
    chunkSize = 0;
    readingChunkSize = true;
}

HttpRequest::Method HttpRequest::getMethod() const {
    return this->method;
}

HttpRequest::Version HttpRequest::getVersion() const {
    return this->version;
}

const std::string& HttpRequest::getTarget() const {
    return this->target;
}

const std::string& HttpRequest::getPath() const {
    return this->path;
}

const std::map<std::string,std::string>& HttpRequest::getQuery() const {
    return queryParams;
}

const std::vector<char>& HttpRequest::getBody() const {
    return body;
}

bool HttpRequest::hasHeader(const std::string& key) const {
    return headers.find(key) != headers.end();
}

const std::string& HttpRequest::getHeader(const std::string& key) const {
    if (this->hasHeader(key))
        return headers.find(key)->second;
    static const std::string empty;
    return empty;
}

const std::map<std::string,std::string>& HttpRequest::getHeaders() const {
    return headers;
}

bool HttpRequest::keepAlive() const
{
    if (hasHeader("connection"))
    {
        if (version == HTTP_1_1)
            return getHeader("connection") != "close";
        else
            return getHeader("connection") == "keep-alive";
    }

    return version == HTTP_1_1;
}
