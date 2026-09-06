#include "WebServer.hpp"
#include "../http/Router.hpp"
#include <cstdlib>
#include <arpa/inet.h>
#include "Connection.hpp"

WebServer::WebServer(ConfigLoader configLoader) {
    this->config = configLoader.load();
    requestHandler = RequestHandler();
}

void WebServer::run() {
    if (setUpSocket())
        return;
    if (setUpEpoll())
        return;
    eventLoop();
    close(serverfd);
}

int WebServer::setUpSocket() {
    serverfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (serverfd == -1)
    {
        std::cout << "socket creation failed" << std::endl;
        return 1;
    }

    int opt = 1;
    if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        std::cout << "setsocket failed.\n";
        close(serverfd);
        return 1;
    }

    sockaddr_in addr = {};

    addr.sin_family = AF_INET;
    
    std::string host = config.servers[0].directives["host"][0];
    addr.sin_addr.s_addr = inet_addr(host.c_str());

    int port = std::atoi(config.servers[0].directives["listen"][0].c_str());
    addr.sin_port = htons(port);

    int bindres = bind(serverfd, (sockaddr *)&addr, sizeof(addr));
    if (bindres == -1)
    {
        std::cout << "bind failed" << std::endl;
        close(serverfd);
        return 1;
    }

    if (listen(serverfd, 1000) == -1)
    {
        std::cout << "listening failed.." << std::endl;
        close(serverfd);
        return 1;
    }

    std::cout << "Server is listening " << " ..." << std::endl;
    return 0;
}

int WebServer::setUpEpoll() {
    epollfd = epoll_create1(EPOLL_CLOEXEC);
    if (epollfd == -1)
    {
        std::cout << "epoll_create failed.." << std::endl;
        close(serverfd);
        return 1;
    }
    epoll_event ev = {};
    ev.events = EPOLLIN;
    ev.data.fd = serverfd;

    int epoll_ctl_res = epoll_ctl(epollfd, EPOLL_CTL_ADD, serverfd, &ev);
    if (epoll_ctl_res == -1)
    {
        std::cout << "epoll_ctr failed" << std::endl;
        close(serverfd);
        return 1;
    }
    return 0;
}

int WebServer::eventLoop() {
    while (true)
    {
        epoll_event events[1024];
        int nfds = epoll_wait(epollfd, events, 1024, -1);
        if (nfds == -1)
        {
            std::cout << "epoll_wait failed" << std::endl;
            close(serverfd);
            return 1;
        }
        for (int i = 0; i < nfds; i++)
        {
            if (events[i].data.fd == serverfd)
                acceptConnections();
            else if (events[i].events & EPOLLOUT)
                handleWrite(events[i].data.fd);
            else
                handleClient(events[i]);
        }
    }
    return 1;
}

void WebServer::acceptConnections() {
    while (true) {
        int clientfd = accept(serverfd, 0, 0);
        if (clientfd < 0)
            break;

        int flags = fcntl(clientfd, F_GETFL, 0);

        if (flags == -1 || fcntl(clientfd, F_SETFL, flags | O_NONBLOCK) == -1)
        {
            std::cout << "fcntl failed" << std::endl;
            close(clientfd);
            continue;
        }
        clients.insert(std::make_pair(clientfd, Connection(clientfd)));

        epoll_event client_ev = {};
        client_ev.events = EPOLLIN;
        client_ev.data.fd = clientfd;
        int epoll_ctl_res = epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &client_ev);
        if (epoll_ctl_res == -1)
        {
            std::cout << "epoll_ctr failed" << std::endl;
            close(clientfd);
            clients.erase(clientfd);
            continue;
        }
    }
}

void WebServer::handleClient(epoll_event& event) {
    char buffer[1024];

    int fd = event.data.fd;
    int bytes = read(fd, buffer, 1023);
    std::cout << "bytes = " << bytes << std::endl;
    if (bytes < 0)
    {
        return;
    }
    else if (bytes == 0)
    {
        std::cout << "client disconnected.\n";
        close(fd);
        epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
        clients.erase(fd);
        return;
    }

    std::map<int, Connection>::iterator it = clients.find(fd);

    if (it == clients.end())
    {
        clients.insert(std::make_pair(fd, Connection(fd)));
        it = clients.find(fd);
    }

    Connection &clientConnection =  it->second;
    clientConnection.appendData(buffer, bytes);
    
    if (clientConnection.hasCompleteRequest())
    {
        Router router(config);

        HttpRequest request = clientConnection.getRequest();
        RouteResult routeResult = router.route(request);
        HttpResponse response = requestHandler.handle(request, routeResult);
        if (request.keepAlive())
        {
            response.setHeader("Connection", "keep-alive");
            clientConnection.setKeepAlive(true);
        }
        else
        {
            response.setHeader("Connection", "close");
            clientConnection.setKeepAlive(false);
        }
        clientConnection.setWriteBuffer(response.serialize());

        epoll_event ev = {};
        ev.events = EPOLLOUT;
        ev.data.fd = fd;

        if (epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev) == -1)
        {
            close(fd);
            clients.erase(fd);
        }
    }
}

void WebServer::handleWrite(int fd)
{
    std::cout << "WRITE EVENT fd=" << fd << std::endl;

    std::map<int, Connection>::iterator it = clients.find(fd);
    Connection& connection = it->second;

    const std::vector<char>& buffer = connection.getWriteBuffer();

    size_t offset = connection.getWriteOffset();

    size_t bytes = write(
        fd,
        buffer.data() + offset,
        buffer.size() - offset
    );

    if (bytes < 0)
    {
        close(fd);
        epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
        clients.erase(fd);
        return;
    }

    connection.advanceWriteOffset(bytes);

    if (!connection.hasPendingWrite())
    {
        if (connection.shouldKeepAlive())
        {
            connection.clearWriteBuffer();
            connection.clearRequest();

            epoll_event ev = {};
            ev.events = EPOLLIN;
            ev.data.fd = fd;

            if (epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev) == -1)
            {
                close(fd);
                clients.erase(fd);
            }
        }
        else
        {
            epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
            close(fd);
            clients.erase(fd);
        }
    }
}
