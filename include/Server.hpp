#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <csignal>
#include <fcntl.h>
#include "ConfigFile.hpp"
#include "Constants.hpp"
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

    ConfigFile                  conf;
    int                         epollFd;
    struct epoll_event          event;
    struct epoll_event          events[MAX_EVENTS];
    std::vector<int> 		    serveSocket;
    std::vector<HttpParser>     _requests;
    std::vector<bool>		    _is_used;
    std::vector<int>			_client_activity;
    std::map<int, bool>         _sending;
    std::vector <HttpResponse> _response;


    public:

    Server();
    ~Server();

    void                        initialize(ConfigFile& conf);
    int                         createServerSocket(int port, std::string ipServer);
    void                        run();
    std::vector<int>            getServerSocket();
    bool                        handleClientConnection(int serverIndex, int serverSocket, int i);
    int                         getEpollFd();

    void                        closeServerFd();
    void                        runLoop();
    void                        cleaningServerFd();
    void                        createNewParserObject(size_t index);
    void                        releaseVectors(size_t index);
    std::vector<int>            getClientActivity();
    std::vector<HttpResponse>&  getResponses();
    void                        setClientActivity(int fd);

};

extern Server* g_serverInstance;

#endif
