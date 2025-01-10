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

void printServerConfig(  std::map<std::string, std::map<std::string, LocationConfig>> serverConfig);

class ConfigFile;
class HttpParser;

class Server 
{
    private:

    std::vector<int> serveSocket;
    //int epollfd;
    //int fdClient;
    int fdGeneral;
    int test;


    public:

    Server();
    ~Server();
    
    void initialize(ConfigFile& conf);
    int createServerSocket(int port, std::string ipServer);
    void run(ConfigFile& conf);
    std::vector<int> getServerSocket();
    void handleClientConnection(int serverIndex, ConfigFile& conf, int fdClient, int epollFd);
    int getEpollFd();
    int getClientFd();
    int getfdGeneral();
    void closeServerFd();
    void runLoop(ConfigFile& conf, struct epoll_event* events, struct epoll_event eventint, int epollFd);
    bool isCompleteRequest(const std::string& request);
};

extern Server* g_serverInstance;

#endif