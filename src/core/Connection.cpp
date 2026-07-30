#include "Connection.hpp"

Connection::Connection(int fd): clientFd(fd) {}

void Connection::appendReadData(const char* data, size_t n) {
    request.appendData(data, n);
}

bool Connection::hasCompleteRequest() const {
    return request.isComplete();
}
HttpRequest& Connection::getRequest() {
    return request;
}

void Connection::clearRequest() {
    request.reset();
}

int Connection::getFd() const {
    return clientFd;
}

void Connection::setWriteBuffer(const std::string& data)
{
    writeBuffer = data;
}

const std::string& Connection::getWriteBuffer() const
{
    return writeBuffer;
}   
void Connection::clear() {
    request.reset();
    writeBuffer.clear();
}

void Connection::clearWriteBuffer() {
    writeBuffer.clear();
}