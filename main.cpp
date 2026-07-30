#include <sys/socket.h>
#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <fcntl.h>

int main()
{
    int serverfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
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
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

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

    int epollfd = epoll_create1(EPOLL_CLOEXEC);
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
    while (true)
    {
        std::cout << "Waiting for cllients............" << std::endl;
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
            {
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
                    epoll_event client_ev = {};
                    client_ev.events = EPOLLIN;
                    client_ev.data.fd = clientfd;
                    int epoll_ctl_res = epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &client_ev);
                    if (epoll_ctl_res == -1)
                    {
                        std::cout << "epoll_ctr failed" << std::endl;
                        close(clientfd);
                        continue;
                    }
                }
            }
            else
            {
                char buffer[1024];
                int bytes = read(events[i].data.fd, buffer, 1023);
                if (bytes < 0)
                {
                    continue;
                }
                else if (bytes == 0)
                {
                    std::cout << "client disconnected.\n";
                    close(events[i].data.fd);
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                    continue;
                }
                buffer[bytes] = '\0';
                std::string message =
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: 13\r\n\r\n"
                    "Hello world\r\n";
                int writeres = write(events[i].data.fd, message.c_str(), message.size());
                if (writeres > 0)
                    std::cout << "response successfully sent." << std::endl;
                close(events[i].data.fd);
                epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
            }
        }
    }
    close(serverfd);
    return 0;
}