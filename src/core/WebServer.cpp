#include "WebServer.hpp"
#include "../http/Router.hpp"
#include <cstdlib>
#include <arpa/inet.h>
#include "Connection.hpp"

WebServer::WebServer(ConfigLoader configLoader)
{
    this->config = configLoader.load();
    requestHandler = RequestHandler();
}

void WebServer::run()
{
    if (setUpSocket())
        return;

    if (setUpEpoll())
        return;

    eventLoop();

    for (std::map<int, const Server *>::iterator it = serverfds.begin();
        it != serverfds.end(); ++it)
    {
        close(it->first);
    }

    close(epollfd);
}

int WebServer::setUpSocket()
{
    for (size_t i = 0; i < config.servers.size(); ++i)
    {
        int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

        if (fd == -1)
        {
            std::cout << "socket creation failed" << std::endl;
            return 1;
        }

        int opt = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        {
            std::cout << "setsockopt failed" << std::endl;
            close(fd);
            return 1;
        }

        sockaddr_in addr = {};
        addr.sin_family = AF_INET;

        std::string host =
            config.servers[i].directives["host"][0];

        int port =
            std::atoi(config.servers[i].directives["listen"][0].c_str());

        addr.sin_addr.s_addr = inet_addr(host.c_str());
        addr.sin_port = htons(port);

        if (bind(fd, (sockaddr *)&addr, sizeof(addr)) == -1)
        {
            std::cout << "bind failed on port "
                      << port << std::endl;

            close(fd);
            return 1;
        }

        if (listen(fd, 1000) == -1)
        {
            std::cout << "listen failed on port "
                      << port << std::endl;

            close(fd);
            return 1;
        }

        serverfds[fd] = &config.servers[i];

        std::cout << "Server listening on "
                  << host << ":" << port << std::endl;
    }

    return 0;
}

int WebServer::setUpEpoll()
{
    epollfd = epoll_create1(EPOLL_CLOEXEC);

    if (epollfd == -1)
    {
        std::cout << "epoll_create failed.." << std::endl;

        for (std::map<int, const Server *>::iterator it = serverfds.begin();
             it != serverfds.end(); ++it)
        {
            close(it->first);
        }

        return 1;
    }

    for (std::map<int, const Server *>::iterator it = serverfds.begin();
         it != serverfds.end(); ++it)
    {
        int listenfd = it->first;

        epoll_event ev = {};
        ev.events = EPOLLIN;
        ev.data.fd = listenfd;

        if (epoll_ctl(
                epollfd,
                EPOLL_CTL_ADD,
                listenfd,
                &ev) == -1)
        {
            std::cout << "epoll_ctl failed" << std::endl;

            close(listenfd);
            return 1;
        }
    }

    return 0;
}

bool WebServer::isListeningSocket(int fd) const
{
    return serverfds.find(fd) != serverfds.end();
}

int WebServer::eventLoop()
{
    while (true)
    {
        epoll_event events[1024];

        int nfds = epoll_wait(
            epollfd,
            events,
            1024,
            -1);

        if (nfds == -1)
        {
            std::cout << "epoll_wait failed" << std::endl;
            return 1;
        }

        for (int i = 0; i < nfds; ++i)
        {
            int fd = events[i].data.fd;

            if (isListeningSocket(fd))
                acceptConnections(fd);
            else if (events[i].events & EPOLLOUT)
                handleWrite(fd);
            else if (events[i].events & EPOLLIN)
                handleClient(events[i]);
        }
    }

    return 0;
}

void WebServer::acceptConnections(int listenfd)
{
    while (true)
    {
        int clientfd = accept(listenfd, 0, 0);

        if (clientfd < 0)
            break;

        int flags = fcntl(clientfd, F_GETFL, 0);

        if (flags == -1 || fcntl(clientfd, F_SETFL, flags | O_NONBLOCK) == -1)
        {
            std::cout << "fcntl failed" << std::endl;
            close(clientfd);
            continue;
        }

        std::map<int, const Server *>::const_iterator serverIt = serverfds.find(listenfd);

        if (serverIt == serverfds.end())
        {
            close(clientfd);
            return;
        }

        const Server *server = serverIt->second;
        clients.insert(std::make_pair(clientfd, Connection(clientfd, server)));

        epoll_event client_ev = {};
        client_ev.events = EPOLLIN;
        client_ev.data.fd = clientfd;

        if (epoll_ctl(
                epollfd,
                EPOLL_CTL_ADD,
                clientfd,
                &client_ev) == -1)
        {
            std::cout << "epoll_ctl failed" << std::endl;

            close(clientfd);
            clients.erase(clientfd);
            continue;
        }
    }
}

void WebServer::handleClient(epoll_event &event)
{
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
        std::cerr << "ERROR: client connection not found\n";
        close(fd);
        epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
        return;
    }


    Connection &clientConnection = it->second;
    const Server *server = clientConnection.getServer();
    if (server == NULL)
    {
        std::cerr << "ERROR: client has no server\n";
        return;
    }
    clientConnection.appendData(buffer, bytes);

    if (clientConnection.hasCompleteRequest())
    {
        Router router(config);

        HttpRequest request = clientConnection.getRequest();
        RouteResult routeResult = router.route(request, *server);
        
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
    Connection &connection = it->second;

    const std::vector<char> &buffer = connection.getWriteBuffer();

    size_t offset = connection.getWriteOffset();

    size_t bytes = write(
        fd,
        buffer.data() + offset,
        buffer.size() - offset);

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
