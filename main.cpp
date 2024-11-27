#include "ConfigFile.hpp"
#include "Server.hpp"

#define PORT 8080


int main(int ac, char **av) 
{
    if (ac != 2) {
        std::cout << "Too many arguments" << std::endl;
        return -1;
    }
    ConfigFile serverFile(av[1]);
    if (!serverFile.parseServerParams()) 
    {
        std::cerr << "Error parsing server parameters." << std::endl;
        return -1;
    }
    //serverFile.printParam();
    Server  server(serverFile.getPort(), serverFile.getIpServer(), serverFile.getServerName());


    if (!server.initialize())
    {
        std::cout << "error to initialize\n";
        return 1;
    }

    server.run();

    return 0;
}