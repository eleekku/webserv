#include "../include/Server.hpp"
#include "../include/HttpResponse.hpp"

#define MAX_EVENTS 10
#define BUFFER_SIZE 10000



//note : try catch to handle errors
//       check max client body

Server* g_serverInstance = nullptr;

Server::Server(){  test = 0; }

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
        throw std::runtime_error("createServerSocket = Error open a socket server");
    }
    if (setNonBlocking(fds) == -1)
    {
        throw std::runtime_error("createServerSocket = setNonBlocking");
    }
    int opt = 1;
    if (setsockopt(fds, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        close(fds);
        throw std::runtime_error("createServerSocket = error setting SO_REUSEADDR");
    }
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ipServer.c_str());
    if (bind(fds, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        throw std::runtime_error(" CreateServerSocket bind fail");
    }
    if (listen(fds, 10) < 0)
    {
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

void Server::run(ConfigFile& conf) //need to spit
{
    std::cout << "Server running. Waiting for connections..." << std::endl;

    struct epoll_event event, events[MAX_EVENTS];//MAX_EVENTS depende de cuanta carga tendra el servidor pero mas grande sea este numero mas recurso tomara del sistema
    int epollFd = epoll_create1(0);
    if (epollFd == -1)
    {
        closeServerFd();
        throw std::runtime_error("run = Error creating epoll instance");
    }
    //epollfd = epollFd;
    g_serverInstance = this;
    int socketSize = serveSocket.size();
    for (int i = 0; i < socketSize; ++i)
    {
        event.events = EPOLLIN | EPOLLET; // Non-blocking edge-triggered
        event.data.u32 = (i << 16) | serveSocket[i];
        if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serveSocket[i], &event) == -1)
        {
            closeServerFd();
            close(epollFd);
            throw std::runtime_error("run = Error adding server socket to epoll");
        }
    }
    runLoop(conf, &events[MAX_EVENTS], event, epollFd);
}

void Server::runLoop(ConfigFile& conf, struct epoll_event* events, struct epoll_event event, int epollFd)
{
    while (true)
    {
        int nfds = epoll_wait(epollFd, events, MAX_EVENTS, -1);
        if (nfds == -1) 
        {
            closeServerFd();
            close(epollFd);
            throw std::runtime_error("run = Error in epoll_wait");
        }
        for (int i = 0; i < nfds; ++i)
        {
            int currentData = events[i].data.u32;
            int serverIndex = currentData >> 16;
            int fdCurrentData = currentData & 0xFFFF;

            if (std::find(serveSocket.begin(), serveSocket.end(), fdCurrentData) != serveSocket.end())
            {
                // New connection on server socket
                int clientFd;
                sockaddr_in clientAddr{};
                socklen_t clientLen = sizeof(clientAddr);
                clientFd = accept(fdCurrentData, (sockaddr*)&clientAddr, &clientLen);
                if (clientFd == -1) 
                {
                    std::cerr << "Accept failed\n";
                    continue;
                }
                //fdClient = clientFd;
                g_serverInstance = this;
                if (setNonBlocking(clientFd) == -1)
                {
                    close(clientFd);
                    continue;
                }
                // Associate client with server index
                event.events = EPOLLIN;
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
                handleClientConnection(serverIndex, conf, fdCurrentData, epollFd);
            }
        }
    }
    close(epollFd);
    for (int fd : serveSocket) 
    {
        close(fd);
    }
}

void Server::handleClientConnection(int serverIndex, ConfigFile& conf, int serverSocket, int epollFd) // tengo que hacer los epoll event EPOLLIN y EPOLLOUT
{
    // Data available on client socket
    char buffer[BUFFER_SIZE] = {0};

    while (true)
    {
        long bytesRead = 0;
        bytesRead = recv(serverSocket, buffer, sizeof(buffer) - 1, 0); // si intento leer del mismo socker dos veces cuando a la primera ya se lee todo da un numero raro.
        buffer[bytesRead] = '\0'; //hay que trabajar leer todo lo que hay en el tener toda la data antes de procesar request y responder.
        std::cout << "\n\n bytesRead =\n" << serverSocket << "\n"  << bytesRead <<  "\n\n";
        if (bytesRead <= 0) 
        {
            if (bytesRead < 0)
            {
                 throw (std::runtime_error("error en recv\n"));
            }
            epoll_ctl(epollFd, EPOLL_CTL_DEL, serverSocket, nullptr);
            close(serverSocket);
            return ;
        }
        else
        {

            HttpParser request(bytesRead);
            request.parseRequest(buffer);
            std::cout << "\nserver index = " << serverIndex << "\n";
            HttpResponse response = receiveRequest(request, conf, serverIndex);
            std::string body = response.generate(); 
            ssize_t bytesSent = send(serverSocket, body.c_str(), body.size(), MSG_NOSIGNAL);
            if (bytesSent == -1) 
            {
                std::cerr << "Error sending response to client.\n";
            }
            epoll_ctl(epollFd, EPOLL_CTL_DEL, serverSocket, nullptr);
            close(serverSocket);
            return ;
        }
    }
}


std::vector<int>  Server::getServerSocket() { return serveSocket; }

int Server::getfdGeneral() { return fdGeneral; }
