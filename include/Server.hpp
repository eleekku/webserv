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
    std::map <int, HttpResponse> _response;


    public:

    Server();
    ~Server();

    //sever
    void                        initialize(ConfigFile& conf);;
    void                        run();
    bool                        handleClientConnection(int serverIndex, int serverSocket, int i);
    void                        runLoop();
    
    //Utilities
    int                         createServerSocket(int port, std::string ipServer);
    void                        closeServerFd();
    void                        cleaningServerFd();
    void                        createNewParserObject(size_t index);
    void                        releaseVectors(size_t index);
    void                        setClientActivity(int fd);

    //Getter
    int                         getEpollFd();
    std::vector<int>            getClientActivity();
    std::vector<int>            getServerSocket();
    std::map <int, HttpResponse>&  getResponses();

};

extern Server* g_serverInstance;

#endif
