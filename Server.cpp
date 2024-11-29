#include "Server.hpp"
#include <iostream>
#include <arpa/inet.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <fcntl.h>

#define MAX_EVENTS 10
#define BUFFER_SIZE 1024

int setNonBlocking(int fd) 
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) 
    {
        perror("fcntl");
        return(-1);
    }
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1) {
        perror("fcntl");
        return(-1);
    }
    return 0;
}

Server::Server(int portServer, std::string ipserver, std::string servername)  
{
    port = portServer;
    ipServer = ipserver;
    serverName = servername;
    serveSocket = -1;
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

void Server::run()
{
    std::cout << "Server running. Waiting for connections..." << std::endl;

    struct epoll_event event, events[MAX_EVENTS];

    int epollFd = epoll_create1(0);
    if (epollFd == -1)
    {
        std::cout << "epoll created1\n";
        return ;
    }
    event.events = EPOLLIN;
    event.data.fd = serveSocket;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serveSocket, &event) == -1) {
        std::cout << "epoll_ctl\n";
        return ;
    }


    while (true) 
    {
        int nfds = epoll_wait(epollFd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            std::cout << "epoll_wait\n";
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
                    std::cout << "setNonBlocking cliente fail\n";
                    close(client_fd);
                    continue;
                }

                event.events = EPOLLIN | EPOLLET;
                event.data.fd = client_fd;
                if (epoll_ctl(epollFd , EPOLL_CTL_ADD, client_fd, &event))
                {
                    std::cout << "epoll_ctl client\n";
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
                    ssize_t nl = buffer.find("\n");
                    std::string test1 = buffer.substr(0, nl);
                    std::cout << "firsh line of client request----------------\n"<< test1 << "\n" << "-------------\n";
                    buffer.resize(bytesRead);
                    std::cout << "Received: " << buffer << std::endl;

                    std::string httpResponse = "HTTP/1.1 200 OK\r\n"
                                                "Content-Type: text/plain\r\n"
                                                "Content-Length: " + std::to_string(bytesRead) + "\r\n"
                                                "\r\n" +
                                                buffer;

                    ssize_t bytesSent = send(client_fd, httpResponse.c_str(), httpResponse.size(), MSG_NOSIGNAL);
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