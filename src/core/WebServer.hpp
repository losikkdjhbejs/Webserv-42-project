#include "../config/ConfigLoader.hpp"
#include "Connection.hpp"
#include <map>
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <fcntl.h>

// Owns the server infrastructure:
// configuration, listening sockets,
// epoll instance and all active connections.

class Server {
    private:
        ConfigLoader config;
        int serverfd;
        int epollfd; 
        std::map<int, Connection> clients;
        int  setUpSocket();
        int setUpEpoll();
        void eventLoop();
        void acceptConnections();
        void handleClient(epoll_event& event);
    public:
        Server(ConfigLoader ConfigLoader);
        void run();
};