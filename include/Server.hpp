#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <iostream>
#include <arpa/inet.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <fcntl.h>
#include "../include/ConfigFile.hpp" 
#include "../include/HttpParser.hpp"

class ConfigFile;
class HttpParser;

class Server {
private:
    int port;
    int serveSocket;
    std::string ipServer;
    std::string serverName;
    std::string maxBody;
    std::string errorPage;
    std::map<std::string, LocationConfig> serverConfig;
    std::vector<int> indexLocation;


public:

    Server(ConfigFile& conf, int i);
    ~Server();
    bool initialize();
    void run(ConfigFile &confile);

};


#endif