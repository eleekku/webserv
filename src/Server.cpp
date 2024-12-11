#include "../include/Server.hpp"
#include "../include/HttpResponse.hpp"

#define MAX_EVENTS 10
#define BUFFER_SIZE 1024



//note : try catch to handle errors
//       study code part how i got the indexsever   
//       check max client body

Server* g_serverInstance = nullptr;

Server::Server(){}

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
        std::cout << "socket error\n";
        return -1;
    }
    if (setNonBlocking(fds) == -1)
    {
        std::cout << "setNonBlocking cliente fail\n";
        return -1;
    }
    int opt = 1;
    if (setsockopt(fds, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) 
    {
        std::cout << "Error setting SO_REUSEADDR\n";
        close(fds);
        return -1;
    }
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ipServer.c_str());    
    if (bind(fds, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) 
    {
        std::cout << "bind fail\n";
        return -1;
    }
    if (listen(fds, 10) < 0) 
    {
        std::cout << "listen fail\n";
        return -1;
    }
    return fds;
}

bool Server::initialize(ConfigFile& conf)
{
    std::vector<std::string> ips = conf.getIpServer();
    std::vector<int> portServer = conf.getPort();
    int sizeIP = ips.size();
    for (int i = 0; i < sizeIP; i++)
    {
        int serverFd = createServerSocket(portServer[i], ips[i]);
        if (serverFd == -1)
        {
            std::cout << "serverFd fail\n";
            return false;
        }
        serveSocket.push_back(serverFd);
    }
    g_serverInstance = this;
    for (int i = 0; i < sizeIP; i++)
    { 
        std::cout << "Server [" << i + 1 << "] initialized on " << ips[i] << ":" << portServer[i] << std::endl;
    }
    return true;
}

void Server::run(ConfigFile& conf) //need to spit 
{
    std::cout << "Server running. Waiting for connections..." << std::endl;

    struct epoll_event event, events[MAX_EVENTS];
    int epollFd = epoll_create1(0);
    if (epollFd == -1) 
    {
        std::cout << "Error creating epoll instance\n";
        return;
    }
    pollfd = epollFd;
    g_serverInstance = this;
    int socketSize = serveSocket.size();
    for (int i = 0; i < socketSize; ++i) 
    {
        event.events = EPOLLIN | EPOLLET; // Non-blocking edge-triggered
        event.data.u32 = (i << 16) | serveSocket[i];
        if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serveSocket[i], &event) == -1) 
        {
            std::cerr << "Error adding server socket to epoll\n";
            close(epollFd);
            return;
        }
    }
    while (true) 
    {
        int nfds = epoll_wait(epollFd, events, MAX_EVENTS, -1);
        if (nfds == -1) 
        {
            std::cerr << "Error in epoll_wait\n";
            break;
        }
        for (int i = 0; i < nfds; ++i) 
        {
            int currentData = events[i].data.u32;
            int serverIndex = currentData >> 16;
            int fd = currentData & 0xFFFF;
            fdGeneral = fd;

            if (std::find(serveSocket.begin(), serveSocket.end(), fd) != serveSocket.end()) 
            {
                // New connection on server socket
                sockaddr_in clientAddr{};
                socklen_t clientLen = sizeof(clientAddr);
                int clientFd = accept(fd, (sockaddr*)&clientAddr, &clientLen);
                if (clientFd == -1) 
                {
                    std::cerr << "Accept failed\n";
                    continue;
                }
                fdClient = clientFd;
                g_serverInstance = this;
                if (setNonBlocking(clientFd) == -1) 
                {
                    close(clientFd);
                    continue;
                }
                // Associate client with server index
                event.events = EPOLLIN | EPOLLET;
                event.data.u32 = (serverIndex << 16) | clientFd;
                if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &event) == -1) 
                {
                    std::cerr << "Failed to add client to epoll\n";
                    close(clientFd);
                    continue;
                }
                std::cout << "Accepted connection on server " << serverIndex << "\n";
            } 
            else 
            {
                // Data available on client socket
                char buffer[BUFFER_SIZE];
                ssize_t bytesRead = recv(fd, buffer, sizeof(buffer) - 1, 0);
                if (bytesRead <= 0) 
                {
                    close(fd);
                    epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr);
                } 
                else 
                {
                    HttpParser request(bytesRead);
                    request.parseRequest(buffer);

                    std::cout << "Request method: " << request.getMethodString() << std::endl;

                    //elias serverIndex lets u know what server have have to response.
                    std::cout << "\nserver index = " << serverIndex << "\n";
                    HttpResponse response = receiveRequest(request, conf);
                    std::string body = response.generate();

                    ssize_t bytesSent = send(fd, body.c_str(), body.size(), MSG_NOSIGNAL);
                    if (bytesSent == -1) 
                    {
                        std::cerr << "Error sending data to client." << std::endl;
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

std::vector<int>  Server::getServerSocket() { return serveSocket; }

int Server::getEpollFd() { return pollfd; }

int Server::getClientFd() { return fdClient; }

int Server::getfdGeneral() { return fdGeneral; }
