#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <iostream>
#include <arpa/inet.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <csignal>
#include <fcntl.h>
#include "../include/ConfigFile.hpp" 
#include "../include/HttpParser.hpp"

class ConfigFile;
class HttpParser;

class Server {
private:
    //int port;
    std::vector<int> serveSocket;


public:

    Server();
    ~Server();
    bool initialize(ConfigFile& conf);
    int create_server_socket(int port, std::string ipServer);
    void run(ConfigFile& conf);
    std::vector<int> getServerSocket();
    void handleClientConnection(int clientFd, int serverIndex, ConfigFile& conf);


};

extern Server* g_serverInstance;

#endif