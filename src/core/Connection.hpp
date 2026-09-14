#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <iostream>
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../config/Server.hpp"
#include <vector>
#include <string>

class Connection
{
private:
    int clientFd;
    const Server *server;
    std::vector<char> writeBuffer;
    size_t writeOffset;
    HttpRequest request;
    bool keepAlive;
public:
    Connection();
    Connection(int fd, const Server *server);
    const Server *getServer() const;
    void appendData(const char *data, size_t n);
    bool hasCompleteRequest() const;
    HttpRequest &getRequest();
    void clearRequest();
    int getFd() const;
    void setWriteBuffer(const std::vector<char> &data);
    const std::vector<char> &getWriteBuffer() const;
    void clear();
    void clearWriteBuffer();
    void advanceWriteOffset(size_t n);
    bool hasPendingWrite() const;
    size_t getWriteOffset() const;
    void setKeepAlive(bool value);
    bool shouldKeepAlive() const;
};

#endif