#include "../include/ConfigFile.hpp"
#include "../include/Server.hpp"

void globalSignalHandler(int signum) 
{
    if (g_serverInstance != nullptr) 
    {
        for (int socket : g_serverInstance->getServerSocket()) 
        {
            if (socket >= 0) 
            {
                close(socket);
            }
        }
        if (g_serverInstance->getfdGeneral() >= 0) 
        {
            close(g_serverInstance->getfdGeneral());
        }
        if (g_serverInstance->getClientFd() >= 0) 
        {
            close(g_serverInstance->getClientFd());
        }
        if (g_serverInstance->getEpollFd() >= 0) 
        {
            close(g_serverInstance->getEpollFd());
        }
    }

    std::cout << "\nServer shut down.\n" << std::endl;
    exit(signum);
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
        std::cout << "arguments" << std::endl;
        return -1;
    }   
    try 
    {
        ConfigFile serverFile(av[1]);
        serverFile.openConfigFile();
        //serverFile.printParam();  
        //printServerConfig(serverFile.getServerConfig());
        Server  server;
        signal(SIGINT, globalSignalHandler);
        server.initialize(serverFile);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
