#include "../include/ConfigFile.hpp"
#include "../include/Server.hpp"
#include "../include/CgiHandler.hpp"

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
        if (g_serverInstance->getEpollFd() >= 0) 
        {
            close(g_serverInstance->getEpollFd());
        }
    }
    throw std::runtime_error("\nServer shut down.");
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
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
