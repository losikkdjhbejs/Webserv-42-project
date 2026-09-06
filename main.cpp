#include <iostream>
#include "src/core/WebServer.hpp"
#include "src/config/ConfigLoader.hpp"

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cout << "Missing config file --> usage: ./webserv config/demo.conf";
        return 1;
    }

    ConfigLoader configLoader = ConfigLoader(argv[1]);
    WebServer server = WebServer(configLoader);
    server.run();
    (void)argv;
    return 0;
}