#include "../include/Server.hpp"
#include "../include/HttpResponse.hpp"

#define MAX_EVENTS 10
#define BUFFER_SIZE 1024

Server* g_serverInstance = nullptr;

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

Server::Server()  
{
    //port = conf.getPort(i);
    //ipServer = conf.getIpServer(i);
    //serverName = conf.getServerName(i);
    //serveSocket = -1;
    //serverConfig = conf.getLocations();
    //indexLocation = conf.getIndexLocation();
    //maxBody = conf.getMax_body(i);
    //errorPage = conf.getErrorPage(i);

    /*std::cout << "IP Server: " << ipServer << "\n"
                << "Server Name: " << serverName << "\n"
                << "Port: " << port << "\n"
                << "Max Body Size: " << maxBody << "\n"
                << "Error Page: " << errorPage << "\n"
                << "index location: " << indexLocation[i] << "\n";


    std::cout << "Server location:\n";
    for (const auto& [location, config] : serverConfig) {
        std::cout << "Location: " << location << "\n";
        std::cout << "  limit_except: " << config.limit_except << "\n";
        std::cout << "  root: " << config.root << "\n";
        std::cout << "  autoindex: " << (config.autoindex ? "on" : "off") << "\n";
        std::cout << "  index: " << config.index << "\n";
    }*/
}

Server::~Server(){}

int Server::create_server_socket(int port, std::string ipServer)
{
    int fds;
    fds = socket(AF_INET, SOCK_STREAM, 0);
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
    /*serveSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serveSocket < 0)
    {
        std::cout << "socket error\n";
        return false;
    }

    if (setNonBlocking(serveSocket) == -1)
    {
        std::cout << "setNonBlocking cliente fail\n";
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ipServer.c_str());
    
    if (bind(serveSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) 
    {
        std::cout << "bind fail\n";
        return false;
    }

    if (listen(serveSocket, 10) < 0) 
    {
        std::cout << "listen fail\n";
        return false;
    }*/
    std::vector<std::string> ips = conf.getIpServer();
    std::vector<int> portServer = conf.getPort();
    int sizeIP = ips.size();
    for (int i = 0; i < sizeIP; i++)
    {
        int serverFd = create_server_socket(portServer[i], ips[i]);
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

void Server::run(ConfigFile& conf) 
{
    std::cout << "Server running. Waiting for connections..." << std::endl;

    struct epoll_event event, events[MAX_EVENTS];
    int epollFd = epoll_create1(0);
    if (epollFd == -1) 
    {
        std::cout << "Error creating epoll instance\n";
        return;
    }
    int socketSize = serveSocket.size();
    for (int i = 0; i < socketSize; ++i) 
    {
        event.events = EPOLLIN;
        event.data.fd = serveSocket[i];
        if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serveSocket[i], &event) == -1) 
        {
            std::cout << "Error adding server socket to epoll\n";
            return;
        }
    }

    while (true) 
    {
        int nfds = epoll_wait(epollFd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            std::cerr << "Error in epoll_wait\n";
            exit(1);
        }

        for (int i = 0; i < nfds; ++i) 
        {
            int currentFd = events[i].data.fd;

            for (int j = 0; j < socketSize; ++j) 
            {
                if (currentFd == serveSocket[j]) 
                {
                    sockaddr_in clientAddr{};
                    socklen_t clientLen = sizeof(clientAddr);
                    int clientFd = accept(serveSocket[j], (sockaddr*)&clientAddr, &clientLen);
                    if (clientFd == -1) {
                        std::cerr << "Accept failed on socket " << serveSocket[j] << "\n";
                        continue;
                    }
                    std::cout << "Accepted connection on server " << j << "\n";
                    handleClientConnection(clientFd, j, conf);
                }
            }
        }
    }
    close(epollFd);
    for (int i = 0; i < socketSize; ++i) 
    {
        close(serveSocket[i]);
    }
}

void Server::handleClientConnection(int clientFd, int serverIndex, ConfigFile& conf) 
{
    std::vector <std::string> ips = conf.getIpServer();
    std::vector <int> portServerC = conf.getPort();
    std::cout << "New connection from "
              << ips[serverIndex] << ":"
              << portServerC[serverIndex]
              << " to server " << serverIndex << std::endl;

    std::string buffer;
    buffer.resize(BUFFER_SIZE);
    ssize_t bytesRead = recv(clientFd, &buffer[0], BUFFER_SIZE - 1, 0);
    if (bytesRead == -1) 
    {
        std::cout << "Error receiving data from client" << std::endl;
        close(clientFd);
        return;
    }
    else if (bytesRead == 0) 
    {
        std::cout << "Client disconnected." << std::endl;
        close(clientFd);
    }
    else
    {
        HttpParser request(bytesRead);
        request.parseRequest(buffer);

        std::cout << "Request method: " << request.getMethodString() << std::endl;

        //elias serverIndex lets u know what server have have to response.
        HttpResponse response = receiveRequest(request, conf);
        std::string body = response.generate();

        ssize_t bytesSent = send(clientFd, body.c_str(), body.size(), MSG_NOSIGNAL);
        if (bytesSent == -1) 
        {
            std::cerr << "Error sending data to client." << std::endl;
        }
    }

    close(clientFd);
}

std::vector<int>  Server::getServerSocket()
{
    return serveSocket;
}