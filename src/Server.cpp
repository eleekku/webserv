#include "../include/Server.hpp"
#include "../include/HttpResponse.hpp"


#define MAX_EVENTS 10
#define BUFFER_SIZE 1024

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

Server::Server(ConfigFile& conf, int i)  
{
    port = conf.getPort(i);
    ipServer = conf.getIpServer(i);
    serverName = conf.getServerName(i);
    serveSocket = -1;
    serverConfig = conf.getLocations();
    indexLocation = conf.getIndexLocation();
    maxBody = conf.getMax_body(i);
    errorPage = conf.getErrorPage(i);

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

bool Server::initialize()
{
    serveSocket = socket(AF_INET, SOCK_STREAM, 0);
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
    }

    std::cout << "Server initialized on " << ipServer << ":" << port << std::endl;

    return true;
}

void Server::run() // I will split it.
{
    std::cout << "Server running. Waiting for connections..." << std::endl;

    struct epoll_event event, events[MAX_EVENTS];

    int epollFd = epoll_create1(0);
    if (epollFd == -1)
    {
        std::cout << "error epoll created1\n";
        return ;
    }
    event.events = EPOLLIN;
    event.data.fd = serveSocket;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serveSocket, &event) == -1) {
        std::cout << "error epoll_ctl\n";
        return ;
    }


    while (true) 
    {
        int nfds = epoll_wait(epollFd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            std::cout << "error epoll_wait\n";
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < nfds; i++)
        {
            if (events[i].data.fd == serveSocket) 
            {
                sockaddr_in clientAddr{};
                socklen_t clientLen = sizeof(clientAddr);
                int client_fd = accept(serveSocket, (sockaddr*)&clientAddr, &clientLen);
                if (client_fd == -1) {
                    std::cout << "accept fail\n";
                    continue;
                }
            
                if (setNonBlocking(client_fd) == -1)
                {
                    std::cout << "error setNonBlocking cliente fail\n";
                    close(client_fd);
                    continue;
                }

                event.events = EPOLLIN | EPOLLET;
                event.data.fd = client_fd;
                if (epoll_ctl(epollFd , EPOLL_CTL_ADD, client_fd, &event))
                {
                    std::cout << "erro epoll_ctl client 2\n";
                    close(client_fd);
                    continue;
                }

                std::cout << "New connection from "
                        << inet_ntoa(clientAddr.sin_addr) << ":"
                        << ntohs(clientAddr.sin_port) << std::endl;
            }
            else
            {
                int client_fd = events[i].data.fd;
                std::string buffer;// xabi expect this 
                buffer.resize(BUFFER_SIZE); 
                ssize_t bytesRead = recv(client_fd, &buffer[0], BUFFER_SIZE - 1, 0);
                //check for the body size
                if (bytesRead == -1)
                {
                    std::cout << "Error receiving data from client" << std::endl;
                    close(client_fd);
                    continue;
                }
                else if (bytesRead == 0)
                {
                    // Conexión cerrada
                    std::cout << "Client disconnected." << std::endl;
                    close(client_fd);
                    epoll_ctl(epollFd, EPOLL_CTL_DEL, client_fd, nullptr);
                }
                else
                {
                    HttpParser request(bytesRead);

                    request.parseRequest(buffer);
                    std::cout << "request is:\n" << request.getMethodString() << std::endl;
                    HttpResponse response = receiveRequest(request);
                    std::string body = response.generate();

                    ssize_t bytesSent = send(client_fd, body.c_str(), body.size(), MSG_NOSIGNAL);
                    if (bytesSent == -1)
                    {
                        std::cerr << "Error sending data to client." << std::endl;
                    }
                }
            }
        }
    }
    close(epollFd);
    close(serveSocket);
}