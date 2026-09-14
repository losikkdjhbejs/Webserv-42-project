#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include "../config/ConfigLoader.hpp"
#include "../config/Config.hpp"
#include "../http/RequestHandler.hpp"
#include "Connection.hpp"
#include <map>
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <fcntl.h>


class WebServer {
    private:
        Config config;
        std::map<int, const Server *> serverfds;
        int epollfd;
        RequestHandler requestHandler;
        std::map<int, Connection> clients;
        int  setUpSocket();
        int setUpEpoll();
        int eventLoop();
        void acceptConnections(int listenfd);
        void handleClient(epoll_event& event);
        void handleWrite(int fd);
        bool isListeningSocket(int fd) const;
    public:
        WebServer(ConfigLoader configloader);
        void run();
};

#endif