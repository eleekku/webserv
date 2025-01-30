#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <csignal>
#include <fcntl.h>
#include "ConfigFile.hpp"
#include "HttpParser.hpp"
//#include "HandleRequest.hpp"
//#include "HttpResponse.hpp"

void printServerConfig(  std::map<std::string, std::map<std::string, LocationConfig>> serverConfig);

class ConfigFile;
class HttpParser;
class HttpResponse;

class Server
{
    private:

    std::vector<int> 		serveSocket;
    std::vector<HttpParser> _requests;
    std::vector<bool>		_is_used;
    //ConfigFile &fileC;
    int epollfd;
    int fdClient;
    std::map<int, time_t> client_activity;
    std::map<int, bool> _sending;
    std::vector <HttpResponse> _response;
    size_t totalBytesSent;
    size_t bodySize;
    int contador;
    //int clientFd[10];
    //int fdGeneral;


    public:

    Server();
    ~Server();

    void initialize(ConfigFile& conf);
    int createServerSocket(int port, std::string ipServer);
    void run(ConfigFile& conf);
    std::vector<int> getServerSocket();
    void handleClientConnection(int serverIndex, ConfigFile& conf, int serverSocket, int epollFd, struct epoll_event event, int i);
    int getEpollFd();
    int getClientFd();
    int getfdGeneral();
    void closeServerFd();
    void runLoop(ConfigFile& conf, struct epoll_event* events, struct epoll_event eventint, int epollFd);
    bool isCompleteRequest(const std::string& request);
    void cleaningServerFd();
    std::vector<char> getRequest(int serverSocket, int epollFd);
    void check_inactive_connections(int epollfd);
    void    createNewParserObject(size_t index);
    void    releaseVectors(size_t index);
    //ConfigFile& getFile();
};

extern Server* g_serverInstance;

#endif
