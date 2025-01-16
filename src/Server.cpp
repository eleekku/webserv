#include "../include/Server.hpp"
#include "../include/HttpResponse.hpp"

#define MAX_EVENTS 10

//note : try catch to handle errors
//       check max client body

Server* g_serverInstance = nullptr;

Server::Server(){ }

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
        event.events = EPOLLIN | EPOLLET; // Non-blocking edge-triggered
        event.data.u32 = (i << 16) | serveSocket[i];
        if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serveSocket[i], &event) == -1)
        {
            cleaningServerFd();
            throw std::runtime_error("run = Error adding server socket to epoll");
        }
    }
    runLoop(conf, &events[MAX_EVENTS], event, epollFd);
}
// In this function, fdCurrentClient can be the server's socket or the client's file descriptor since it comes
// from int currentData = events[i].data.u32 (the first 16 bits contain the file descriptor, and the other 16 bits contain the index of the associated server).
void Server::runLoop(ConfigFile& conf, struct epoll_event* events, struct epoll_event event, int epollFd)
{
    while (true)
    {
        int nfds = epoll_wait(epollFd, events, MAX_EVENTS, -1);
        if (nfds == -1)
        {
            cleaningServerFd();
            throw std::runtime_error("run = Error in epoll_wait");
        }
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
                    throw std::runtime_error ("Accept failed\n");
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
                event.events = EPOLLIN;
                event.data.u32 = (serverIndex << 16) | clientFd;
                if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &event) == -1)
                {
                    cleaningServerFd();
                    throw std::runtime_error ("Failed to add client to epoll\n");
                    continue;
                }
                std::cout << "Accepted connection on server " << serverIndex << "\n";
            }
            else
            {
                handleClientConnection(serverIndex, conf, fdCurrentClient, epollFd); // , event);
            }
        }
    }
    close(epollFd);
    for (int fd : serveSocket)
    {
        close(fd);
    }
}

std::vector<char> getRequest(int serverSocket)
{
	int	bytesRead;
	std::vector<char> rawrequest;
	char buffer[BUFFER_SIZE] = {0};

	while (true)
	{
		bytesRead = recv(serverSocket, buffer, BUFFER_SIZE, 0);
		if (bytesRead == -1)
			break;
		// 	throw std::runtime_error("Error reading from client socket");
		// if (bytesRead == 0)
		// 	break;
		rawrequest.insert(rawrequest.end(), buffer, buffer + bytesRead);
		// if (bytesRead < BUFFER_SIZE)
			// break;
	}
	for (char i: rawrequest)
		std::cout << i;
	return rawrequest;
}

void Server::handleClientConnection(int serverIndex, ConfigFile& conf, int serverSocket, int epollFd) // tengo que hacer los epoll event EPOLLIN y EPOLLOUT
{
    std::vector<char> rawrequest;

    rawrequest = getRequest(serverSocket);
    HttpParser request;
    try {
    request.startParsing(rawrequest, serverSocket);
    } catch (std::runtime_error &e) {
		std::cerr << "Error parsing request\n";
	}
    std::cout << "\nserver index = " << serverIndex << "\n";
    HttpResponse response = receiveRequest(request, conf, serverIndex);
    std::string body = response.generate();
    ssize_t bytesSent = send(serverSocket, body.c_str(), body.size(), MSG_NOSIGNAL);
    (void)bytesSent;
    // if (bytesSent == -1)
    // {
    //      if (bytesRead <= 0)
    //     {
    //         if (bytesRead < 0)
    //         {
    //             cleaningServerFd();
    //              throw (std::runtime_error("error en recv\n"));
    //         }
    //         epoll_ctl(epollFd, EPOLL_CTL_DEL, socketClient, nullptr);
    //         close(socketClient);
    //         return ;
    //     }
    //     else
    //     {

    //         HttpParser request(bytesRead);
    //         request.parseRequest(buffer);
    //         std::cout << "\nserver index = " << serverIndex << "\n";
    //         // The response should only be sent after processing all the client's data.
    //         HttpResponse response = receiveRequest(request, conf, serverIndex);

    //         std::string body = response.generate();
    //         size_t totalBytesSent = 0;
    //         size_t bodySize = body.size();

    //         while (totalBytesSent < bodySize)
    //         {
    //             ssize_t bytesSent = send(socketClient, body.c_str() + totalBytesSent, bodySize - totalBytesSent, MSG_NOSIGNAL);
    //             if (bytesSent == -1)
    //             {
    //                 epoll_ctl(epollFd, EPOLL_CTL_DEL, socketClient, nullptr);
    //                 cleaningServerFd();
    //                 throw std::runtime_error ("Error sending response to client.\n");
    //                 return;
    //             }
    //             totalBytesSent += bytesSent;

    //             if (totalBytesSent < bodySize)
    //             {
    //                 // This happens when the response cannot be sent in a single attempt; it needs to be tested and improved.
    //                 event.events = EPOLLOUT;
    //                 event.data.fd = socketClient;
    //                 if (epoll_ctl(epollFd, EPOLL_CTL_MOD, socketClient, &event) == -1)
    //                 {
    //                     cleaningServerFd();
    //                     throw std::runtime_error("\nERROR EPOLLOUT\n");
    //                 }
    //                 return;
    //             }
    //         }
    //         event.events = EPOLLIN;
    //         event.data.fd = socketClient;
    //         if (epoll_ctl(epollFd, EPOLL_CTL_MOD, socketClient, &event) == -1)
    //         {
    //             cleaningServerFd();
    //             throw std::runtime_error("\nERROR EPOLLOUT\n");
    //         }

    //         epoll_ctl(epollFd, EPOLL_CTL_DEL, socketClient, nullptr);
    //         close(socketClient);
    //         return ;
    //     }
    // }
    epoll_ctl(epollFd, EPOLL_CTL_DEL, serverSocket, nullptr);
    close(serverSocket);
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
