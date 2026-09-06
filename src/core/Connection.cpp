#include "Connection.hpp"

Connection::Connection(int fd): clientFd(fd), writeOffset(0), keepAlive(false) {}

void Connection::appendData(const char* data, size_t n) {
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

void Connection::setWriteBuffer(const std::vector<char>& data)
{
    writeBuffer = data;
    writeOffset = 0;
}

const std::vector<char>& Connection::getWriteBuffer() const
{
    return writeBuffer;
}   
void Connection::clear() {
    request.reset();
    writeBuffer.clear();
    writeOffset = 0;
}

void Connection::clearWriteBuffer() {
    writeBuffer.clear();
    writeOffset = 0;
}


void Connection::advanceWriteOffset(size_t n) {
    writeOffset += n;
    if (writeOffset > writeBuffer.size())
        writeOffset = writeBuffer.size();
}

bool Connection::hasPendingWrite() const {
    return writeOffset < writeBuffer.size();
}


size_t Connection::getWriteOffset() const {
    return writeOffset;
}

void Connection::setKeepAlive(bool value)
{
    keepAlive = value;
}

bool Connection::shouldKeepAlive() const
{
    return keepAlive;
}
