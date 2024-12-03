#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <iostream>
#include <arpa/inet.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <fcntl.h>
#include "ConfigFile.hpp" 
#include "../include/HttpParser.hpp"

class ConfigFile;
class HttpParser;

class Server {
private:
    int port;
    int serveSocket;
    std::string ipServer;
    std::string serverName;

public:

    Server(int portServer, std::string ipserver, std::string servername);
    ~Server();
    bool initialize();
    void run();

};

#endif