#include "../include/Server.hpp"
#include "../include/HandleRequest.hpp"
#include "Constants.hpp"
#include <algorithm>
#include <cstddef>

//note : try catch to handle errors
//       check max client body

Server* g_serverInstance = nullptr;

Server::Server()
{
    _response.resize(200);
	_requests.resize(200);
	_is_used.resize(200, false);
}

Server::~Server() {}

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


void Server::initialize(ConfigFile& config)
{
    conf = config;
    std::vector<std::string> ips = config.getIpServer();
    std::vector<int> portServer = config.getPort();
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
    run();
}

void Server::run()
{
    std::cout << "Server running. Waiting for connections..." << std::endl;

    epollFd = epoll_create1(0);
    if (epollFd == -1)
    {
        cleaningServerFd();
        throw std::runtime_error("run = Error creating epoll instance");
    }
    g_serverInstance = this;
    int socketSize = serveSocket.size();
    for (int i = 0; i < socketSize; ++i)
    {
        event.events = EPOLLIN;// | EPOLLET;// | EPOLLOUT; // Non-blocking edge-triggered
        event.data.u32 = (i << 16) | serveSocket[i];
        if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serveSocket[i], &event) == -1)
        {
            cleaningServerFd();
            throw std::runtime_error("run = Error adding server socket to epoll");
        }
    }
    runLoop();
}

/*void Server::check_inactive_connections(int epollFd)//no in use
{
    struct epoll_event event;
    time_t now = time(NULL);
    std::cout << "aqui checking the conecctions\n";
    for (auto it = client_activity.begin(); it != client_activity.end(); )
    {
        if (now - it->second > 10)
        {
            int client_fd = it->first;
            std::cout << "Closing client connection (inactivity): " << client_fd << std::endl;
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

}*/
// In this function, fdCurrentClient can be the server's socket or the client's file descriptor since it comes
// from int currentData = events[i].data.u32 (the first 16 bits contain the file descriptor, and the other 16 bits contain the index of the associated server).

std::vector<int> Server::getClientActivity()
{
	return _client_activity;
}

void Server::runLoop()
{
    int socketS = 0;
    int client = 0;
    while (true)
    {
        std::cout << "Main loop..." << std::endl;
        int nfds = epoll_wait(epollFd, events, MAX_EVENTS, CONNECTION_TIMEOUT);
        if (nfds == -1)
        {
            std::cerr << "\nrun = Error in epoll_wait" << "\n";
            throw std::runtime_error("Error in epoll_wait");
        } else if (nfds == 0)
        {
			for (size_t i = 0; i < _client_activity.size(); i++)
			{
				if (_is_used[_client_activity[i]])
				{
					if (_requests[_client_activity[i]].checkTimeout())
					{
						std::cout << "Timeout reached for client " << i << std::endl;
						epoll_ctl(epollFd, EPOLL_CTL_DEL, _client_activity[i], nullptr);
						close(_client_activity[i]);
						releaseVectors(_client_activity[i]);
					}
				}
			}
		}
        else
        {
            for (int i = 0; i < nfds; i++)
            {
                int currentData = events[i].data.u32;
                int serverIndex = currentData >> 16;
                int fdCurrentData = currentData & 0xFFFF;
                std::cout << "for loop nfds: " << nfds << " and index is " << i <<"\n";
                std::cout << "fdCurrentData: " << fdCurrentData << "\n";
                if (std::find(serveSocket.begin(), serveSocket.end(), fdCurrentData) != serveSocket.end())
                    socketS = fdCurrentData;
                else
                    client = fdCurrentData;
                if (socketS != 0)
                {
                    std::cout << "\nNew connection on server : " << socketS << " lol" << "\n";
                    sockaddr_in clientAddr{};
                    socklen_t clientLen = sizeof(clientAddr);
                    int clientFd = accept(socketS, (sockaddr*)&clientAddr, &clientLen);
                    std::cout << "fd en accept: " << clientFd << "\n";
                    if (clientFd == -1)
                    {
                        std::cerr << "\nAccept failed\n";
                        continue;
                    }
                    g_serverInstance = this;
                    if (setNonBlocking(clientFd) == -1)
                    {
                        std::cerr << "Fail setNonBlocking() in runLoop\n";
                        continue;
                    }
                    // Associate client with server index
                    event.events = EPOLLIN;
                    event.data.fd = clientFd;
                    event.data.u32 = (serverIndex << 16) | clientFd;
                    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &event) == -1)
                    {
                        close(clientFd);
                        std::cerr << "Failed to add client to epoll\n";
                        continue;
                    }
                }
                else
                {
                    if (events[i].events & EPOLLIN)
                    {
                        createNewParserObject(client);
                        if (_requests[client].startParsing(client, conf, serverIndex) == true)
                        {
                            event.events = EPOLLOUT;
                            event.data.fd = client;
                            if (epoll_ctl(epollFd, EPOLL_CTL_MOD, client, &event) == -1)
                            {
                                std::cerr << "Fail epoll_ctl() in parsing\n";
                                continue;
                            }
                        }
                    }
                    else if (events[i].events & EPOLLOUT)
                    {
                       handleClientConnection(serverIndex, client, i);

                    }
                    else if (events[i].events & EPOLLHUP)
                    {
                        std::cout << "came to EPOLLHUP" << "\n";
                        handleClientConnection(serverIndex, client, i);
                    }
                }
                client = 0;
                socketS = 0;
            }
        }
    }
    close(epollFd);
  //  std::cout << "closed epollFD: " << epollFd << "\n";
    for (int fd : serveSocket)
    {
        close(fd);
    }
}

void    Server::createNewParserObject(size_t index)
{
	if (index < _requests.size() && !_is_used[index])
	{
		_is_used[index] = true;
		_requests[index] = HttpParser();
		_client_activity.push_back(index);
	}
}

void Server::releaseVectors(size_t index)
{
	if (index < _requests.size())
	{
		_is_used[index] = false;
		auto new_end = std::remove(_client_activity.begin(), _client_activity.end(), index);
		_client_activity.erase(new_end, _client_activity.end());
	}
}
//this funtion will handle the response for eatch client if its not posible to send the response in ones,
//the response  object will store for later continuos sending the response to that client until everything is sended.
bool Server::handleClientConnection(int serverIndex, int clientFd, int eventIndex)
{
    std::cout << "handleClientConnection cliendfd: " << clientFd << "\n";
    if (_sending.find(clientFd) == _sending.end())
    {
        HttpResponse response;
        response.setEpoll(epollFd);
        receiveRequest(_requests[clientFd], conf, serverIndex, response);
        response.generate();
        if (response.sendResponse(clientFd) != true)
        {
            if (response.checkCgiStatus())
            {
                if(epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, nullptr) == -1)
                {
                    std::cerr << "Fail epoll_ctl() in handleClientConnection\n";
                    return false;
                }
                std::cout << "fdPipe to send: " << response.getFdPipe() << "\n";
                _response[response.getFdPipe()] = response;
                _sending[response.getFdPipe()] = true;
                releaseVectors(eventIndex);
                return false;
            }
            _response[clientFd] = response;
            _sending[clientFd] = true;
            releaseVectors(eventIndex);
            return false;
        }
    }
    else
    {
        auto it = _sending.find(clientFd);
        if (it != _sending.end() && it->second == true)
        {
            if (_response[clientFd].sendResponse(clientFd) != true) {
                return false;
            }
        }
    }
    if (_requests[clientFd].getKeepAlive() == false)
    {
	    if (_response[clientFd].checkCgiStatus())
	        releaseVectors(_response[clientFd].getCgiFdtoSend());
	    releaseVectors(clientFd);
	    std::cout << "deleted in poll" << clientFd << "\n";
	    if(epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, nullptr) == -1)
	    {
	        std::cerr << "Fail epoll_ctl() in handleClientConnection\n";
	        return false;
	    }
	    close(clientFd);
	    _sending.erase(clientFd);
	    std::cout << "\nclosed conection to client \n" << clientFd << "\n";
    } else {
    	_requests[clientFd] = HttpParser();
     	event.events = EPOLLIN;
        event.data.fd = clientFd;
        if (epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &event) == -1)
        	throw std::runtime_error("Error adding client to epoll");
    }
<<<<<<< HEAD
    releaseVectors(clientFd);
    std::cout << "deleted in poll" << clientFd << "\n";
    if(epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, nullptr) == -1)
    {
        std::cerr << "Fail epoll_ctl() in handleClientConnection\n";
        return false;
    }
    close(clientFd);
    _sending.erase(clientFd);
    std::cout << "\nclosed conection to client \n" << clientFd << "\n";
=======
>>>>>>> xavis_cool_parsing
    return true;
}


std::vector<int>  Server::getServerSocket() { return serveSocket; }

int Server::getEpollFd() { return epollFd; }

void Server::cleaningServerFd()
{

    for (int fd : serveSocket)
        close(fd);
    close(epollFd);
}
