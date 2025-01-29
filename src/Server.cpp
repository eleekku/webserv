#include "../include/Server.hpp"
#include "../include/HandleRequest.hpp"
#include <algorithm>
#include <cstddef>

#define MAX_EVENTS 10

//note : try catch to handle errors
//       check max client body

Server* g_serverInstance = nullptr;

Server::Server()  { _response.resize(MAX_EVENTS);
	_requests.resize(MAX_EVENTS);
	_is_used.resize(MAX_EVENTS, false);
}

Server::~Server(){}

int setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        std::cout <<"error fcntl setNonBlocking\n";
        return(-1);
    }
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1) {
        std::cout << "error fcntl setNonBlocking O_NONBLOCK\n";
        return(-1);
    }
    return 0;
}

int Server::createServerSocket(int port, std::string ipServer)
{
    int fds = socket(AF_INET, SOCK_STREAM, 0);
    if (fds < 0)
    {
        cleaningServerFd();
        throw std::runtime_error("createServerSocket = Error open a socket server");
    }
    if (setNonBlocking(fds) == -1)
    {
        cleaningServerFd();
        throw std::runtime_error("createServerSocket = setNonBlocking");
    }
    int opt = 1;
    if (setsockopt(fds, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        cleaningServerFd();
        throw std::runtime_error("createServerSocket = error setting SO_REUSEADDR");
    }
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ipServer.c_str());
    if (bind(fds, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        cleaningServerFd();
        throw std::runtime_error(" CreateServerSocket bind fail");
    }
    if (listen(fds, 10) < 0)
    {
        cleaningServerFd();
        throw std::runtime_error("createServerSocket = listen fail");
    }
    return fds;
}
void Server::closeServerFd()
{
    for (int socket : getServerSocket())
    {
        if (socket >= 0)
            close(socket);
    }
}


void Server::initialize(ConfigFile& conf)
{
    std::vector<std::string> ips = conf.getIpServer();
    std::vector<int> portServer = conf.getPort();
    int sizeIP = ips.size();
    for (int i = 0; i < sizeIP; i++)
    {
        int serverFd = createServerSocket(portServer[i], ips[i]);
        serveSocket.push_back(serverFd);
    }
    g_serverInstance = this;
    for (int i = 0; i < sizeIP; i++)
    {
        std::cout << "Server [" << i + 1 << "] initialized on " << ips[i] << ":" << portServer[i] << std::endl;
    }
    run(conf);
}

void Server::run(ConfigFile& conf)
{
    std::cout << "Server running. Waiting for connections..." << std::endl;

    struct epoll_event event, events[MAX_EVENTS];
    int epollFd = epoll_create1(0);
    if (epollFd == -1)
    {
        cleaningServerFd();
        throw std::runtime_error("run = Error creating epoll instance");
    }
    epollfd = epollFd;
    g_serverInstance = this;
    int socketSize = serveSocket.size();
    for (int i = 0; i < socketSize; ++i)
    {
        event.events = EPOLLIN; //| EPOLLET | EPOLLOUT; // Non-blocking edge-triggered
        event.data.u32 = (i << 16) | serveSocket[i];
        if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serveSocket[i], &event) == -1)
        {
            cleaningServerFd();
            throw std::runtime_error("run = Error adding server socket to epoll");
        }
    }
    runLoop(conf, &events[MAX_EVENTS], event, epollFd);
}

void Server::check_inactive_connections(int epollFd)
{
    time_t now = time(NULL);

    for (auto it = client_activity.begin(); it != client_activity.end(); )
    {
        if (now - it->second > 20)
        {
            int client_fd = it->first;
            std::cout << "Closing client connection (inactivity): " << client_fd << std::endl;

            struct epoll_event event;
            event.data.fd = client_fd;
            epoll_ctl(epollFd, EPOLL_CTL_DEL, client_fd, nullptr);
            close(client_fd);
            it = client_activity.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
// In this function, fdCurrentClient can be the server's socket or the client's file descriptor since it comes
// from int currentData = events[i].data.u32 (the first 16 bits contain the file descriptor, and the other 16 bits contain the index of the associated server).
void Server::runLoop(ConfigFile& conf, struct epoll_event* events, struct epoll_event event, int epollFd)
{
    while (true)
    {
        std::cout << "Main loop..." << std::endl;
        int nfds = epoll_wait(epollFd, events, MAX_EVENTS, 10000);
        if (nfds == -1)
        {
            cleaningServerFd();
            throw std::runtime_error("run = Error in epoll_wait");
        }
        else if (nfds == 0)
        {
            check_inactive_connections(epollfd);
        }
        else
        {
            for (int i = 0; i < nfds; ++i)
            {
                int currentData = events[i].data.u32;
                int serverIndex = currentData >> 16;
                int fdCurrentClient = currentData & 0xFFFF;

                if (std::find(serveSocket.begin(), serveSocket.end(), fdCurrentClient) != serveSocket.end()) // If fdCurrentClient is the server's socket, then there is a new connection to accept; otherwise, fdCurrentClient is the client's file descriptor that needs to be processed.
                {
                    // New connection on server socket
                    int clientFd;
                    sockaddr_in clientAddr{};
                    socklen_t clientLen = sizeof(clientAddr);
                    clientFd = accept(fdCurrentClient, (sockaddr*)&clientAddr, &clientLen);
                    if (clientFd == -1)
                    {
                        cleaningServerFd();
                        std::cerr << "\nAccept failed\n";
                        continue;
                    }
                    fdClient = clientFd;
                    g_serverInstance = this;
                    if (setNonBlocking(clientFd) == -1)
                    {
                        cleaningServerFd();
                        throw std::runtime_error("Fail setNonBlocking() in runLoop\n");
                        continue;
                    }
                    // Associate client with server index
                    events->events = EPOLLIN | EPOLLOUT;
                    event.data.u32 = (serverIndex << 16) | clientFd;
                    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &event) == -1)
                    {
                        cleaningServerFd();
                        throw std::runtime_error ("Failed to add client to epoll\n");
                        continue;
                    }
                    client_activity[fdClient] = time(NULL);
                    std::cout << "Accepted connection on server \n" << serverIndex << "\n" << clientFd << "\n";
                    events[i].events = EPOLLIN;
                    events[i].data.fd = clientFd;
                    epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &event);
                }
                else
                {
                    if (events[i].events & EPOLLIN)
                    {
                        getParser(i);
                        if (_requests[i].startParsing(fdCurrentClient) == true)
                        {
                            events[i].events = EPOLLOUT;
                            events[i].data.fd = fdCurrentClient;
                            epoll_ctl(epollFd, EPOLL_CTL_MOD, fdCurrentClient, &event);
                            std::cout << "Not yet..." << std::endl;
                        }
                    }
                    if (events[i].events & EPOLLOUT)
                    {
                        handleClientConnection(serverIndex, conf, fdCurrentClient, epollFd, event, i);
                    }
                }
            }
        }
    }
    close(epollFd);
    for (int fd : serveSocket)
    {
        close(fd);
    }
}

void    Server::getParser(size_t index)
{
	if (index < _requests.size() && !_is_used[index])
	{
		_is_used[index] = true;
		_requests[index] = HttpParser();
	}
}

void Server::releaseVectors(size_t index)
{
	if (index < _requests.size())
		_is_used[index] = false;
}

void Server::handleClientConnection(int serverIndex, ConfigFile& conf, int serverSocket, int epollFd, struct epoll_event event,  int eventIndex) // tengo que hacer los epoll event EPOLLIN y EPOLLOUT
{

    //std::cout << request[eventIndex].getMethodString() << " " << request[eventIndex].getTarget() << std::endl;
    //std::cout << "\nserver index = " << serverIndex << "\n";
    if (_sending.find(eventIndex) == _sending.end())
    {
        HttpResponse response;
        response = receiveRequest(_requests[eventIndex], conf, serverIndex);
        response.generate();
        if (response.sendResponse(serverSocket, eventIndex) != true)//getStatus for to check if we already send evething
        {
            _response[eventIndex] = response;
            _sending[serverSocket] = true;
            return ;
        }
    }
    else
    {
        auto it = _sending.find(serverSocket);
        if (it != _sending.end() && it->second == true)
        {
            if (_response[eventIndex].sendResponse(serverSocket, eventIndex) != true)
                return ;
        }
    }
    /*
        HttpResponse response = receiveRequest(request, conf, serverIndex);
        body = response.generate();
        bodyready[serverSocket] = body;
        totalBytesSent = 0;
        bodySize = body.size();
    */
    releaseVectors(eventIndex);
    _sending.erase(serverSocket);
    event.data.fd = serverSocket;
    epoll_ctl(epollFd, EPOLL_CTL_DEL, serverSocket, nullptr);
    close(serverSocket);
    client_activity.erase(serverSocket);
    return ;
}


std::vector<int>  Server::getServerSocket() { return serveSocket; }

int Server::getEpollFd() { return epollfd; }


int Server::getClientFd() { return fdClient; }

void Server::cleaningServerFd()
{

    for (int fd : serveSocket)
        close(fd);
    close(fdClient);
    close(epollfd);
}
