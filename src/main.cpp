#include "../include/ConfigFile.hpp"
#include "../include/Server.hpp"

void globalSignalHandler(int signum) 
{
    std::cout << "\nClosing server..." << std::endl;

    if (g_serverInstance != nullptr) 
    {

        for (int socket : g_serverInstance->getServerSocket()) 
        {
            close(socket);
            std::cout << "Socket " << socket << " closed." << std::endl;
        }
    }
    exit(signum);
}

void printServerConfig(  std::map<std::string, std::map<std::string, LocationConfig>> serverConfig) 
{
    for (const auto& server : serverConfig) 
    {
        const std::string& serverKey = server.first;
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
    if (ac != 2) {
        std::cout << "arguments" << std::endl;
        return -1;
    }
    ConfigFile serverFile(av[1]);
    try {
    serverFile.openConfigFile();
    }
    catch(const std::exception& e){

        std::cout << e.what() << std::endl;
    }
    //serverFile.printParam();
    Server  server;

    //printServerConfig(serverFile.getServerConfig());
    if (!server.initialize(serverFile))
    {
        std::cout << "error to initialize\n";
        return 1;
    }
    signal(SIGINT, globalSignalHandler);
    server.run(serverFile);

    return 0;
}
