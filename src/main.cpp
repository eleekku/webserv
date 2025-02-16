#include "../include/ConfigFile.hpp"
#include "../include/Server.hpp"
#include "../include/CgiHandler.hpp"
#include "../include/HttpResponse.hpp"

Server* g_serverInstance = nullptr;

void globalSignalHandler(int signum) 
{
    (void)signum;
//    std::cerr << "\nSignal " << signum << " received. Shutting down server...\n";
    if (g_serverInstance != nullptr) 
    {
        // Close all server sockets
        for (int socket : g_serverInstance->getServerSocket()) 
        {
            if (socket >= 3) 
            {
                close(socket);
            }
        }
        // Close epoll file descriptor
        if (g_serverInstance->getEpollFd() >= 3) 
        {
            close(g_serverInstance->getEpollFd());
        }
        // Terminate all CGI processes
        std::vector <HttpResponse>& responses = g_serverInstance->getResponses();
        for (auto& response : responses) 
        {
    //        std::cout << "response number " << response.getFdPipe() << "\n";
            if (response.checkCgiStatus()) 
            {
                response.terminateCgi();
            }
        }
        std::vector <int> _client_activity = g_serverInstance->getClientActivity();
       // std::vector<int> serverSocket = g_serverInstance->getServerSocket();
        int size = g_serverInstance->getClientActivity().size();
	    for (int i = 0; i < size; i++)
	    {
	        //epoll_ctl(g_serverInstance->getEpollFd(), EPOLL_CTL_DEL, _client_activity[i], nullptr);
            if(_client_activity[i] > 3)
            {
			    close(_client_activity[i]);
            }
			//g_serverInstance->releaseVectors(_client_activity[i]);
	    }
	
   //     close(g_serverInstance->getEpollFd());
  //  std::cout << "closed epollFD: " << epollFd << "\n";
  /*      for (int fd : serverSocket)
        {
        close(fd);
        }
    } */
  //  std::cerr << "throwing from signal\n";   
  
    }
    throw std::runtime_error("\nServer shut down.");
}


void printServerConfig(  std::map<int, std::map<std::string, LocationConfig>> serverConfig) 
{
    for (const auto& server : serverConfig) 
    {
        const int& serverKey = server.first;
        const auto& locations = server.second;

        std::cout << "Server: " << serverKey << "\n";

        for (const auto& location : locations) 
        {
            const std::string& locationKey = location.first;
            const LocationConfig& config = location.second;

            std::cout << "  Location: " << locationKey << "\n";
            std::cout << "    Limit Except: " << config.limit_except << "\n";
            std::cout << "    Root: " << config.root << "\n";
            std::cout << "    Autoindex: " << (config.autoindex ? "true" : "false") << "\n";
            std::cout << "    Index: " << config.index << "\n";
        }
    }
}

int main(int ac, char **av) 
{
    if (ac != 2) 
    {
        std::cout << "sintaxis : ./webserver [configfile path]" << std::endl;
        return 1;
    }   
    try 
    {
        ConfigFile serverFile(av[1]);
        serverFile.openConfigFile();
        serverFile.finalCheck();
        /*std::cout << "\n-------------------\n";
        serverFile.printParam();  
        std::cout << "\n-------------------\n";
        printServerConfig(serverFile.getServerConfig());*/
        std::cout << "Welcome to Server Red Oscura\n";
        //CgiHandler cgi;
        //cgi.executeCGI("cgi-bin/script.py", "", "",);
        Server  server;
        signal(SIGINT, globalSignalHandler);
        server.initialize(serverFile);
    }
    catch(const std::exception& e)
    {
        std::cerr << "here\n";
        std::cerr << e.what() << std::endl;
        return 1;
    }
    std::cerr << "byeee\n";
    return 0;
}
