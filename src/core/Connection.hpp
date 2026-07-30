#include <iostream>
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include <vector>
#include <string>

// Owns the lifetime of one TCP client.
// Stores everything that must survive between epoll events.
class Connection {
    private:
        std::vector<char> writeBuffer;
        int clientFd;
        HttpRequest request;
    public:
        Connection(int fd);
        void appendReadData(const char* data, size_t n);
        bool hasCompleteRequest() const;
        HttpRequest& getRequest();
        void clearRequest();
        int getFd() const;
        void setWriteBuffer(const std::vector<char>& data);
        const std::vector<char>& getWriteBuffer() const;
        void clear();
        void clearWriteBuffer();
};