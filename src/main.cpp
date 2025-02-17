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
            if (response.checkCgiStatus()) 
            {
                response.terminateCgi();
            }
        }
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
        std::cout << "Welcome to Server Red Oscura\n";
        Server  server;
        signal(SIGINT, globalSignalHandler);
        server.initialize(serverFile);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    std::cout << "Server shut down successfully\n";
    return 0;
}
