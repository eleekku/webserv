#include "../include/ConfigFile.hpp"
#include "../include/Server.hpp"
#include "../include/CgiHandler.hpp"
#include "../include/HttpResponse.hpp"

Server* g_serverInstance = nullptr;

void globalSignalHandler(int signum) 
{
    (void)signum;
    if (g_serverInstance != nullptr) 
    {
        // Terminate all CGI processes
        std::map<int, HttpResponse>& responses = g_serverInstance->getResponses();
        for (auto& [key, response] : responses)
        {
            if (response.checkCgiStatus()) 
            {
                response.terminateCgi();
            }
        }
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
        // Close all client sockets
        std::vector <int> _client_activity = g_serverInstance->getClientActivity();
        int size = g_serverInstance->getClientActivity().size();
	    for (int i = 0; i < size; i++)
	    {
            if(_client_activity[i] > 3)
            {
			    close(_client_activity[i]);
            }
	    }
    }
}

int main(int ac, char **av) 
{
    if (ac != 2) 
    {
        std::cout << "syntax : ./webserver [configfile path]" << std::endl;
        return 1;
    }   
    try 
    {
        ConfigFile serverFile(av[1]);
        serverFile.openConfigFile();
        serverFile.finalCheck();
        Server  server;
        signal(SIGINT, globalSignalHandler);
        server.initialize(serverFile);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        std::cout << "Server shut down\n"; 
        return 1;
    }
    return 0;
}
