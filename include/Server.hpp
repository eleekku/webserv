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
    std::vector<int> port;
    std::vector<int> serveSocket;
    std::string ipServer;
    std::string serverName;
    const ConfigFile& confFile;

public:

    Server(const ConfigFile& confFile );
    ~Server();
    bool initialize();
    void run();

};

#endif