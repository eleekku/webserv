#include "../include/ConfigFile.hpp"
#include "../include/Server.hpp"


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
    Server  server(serverFile, 0);


    if (!server.initialize())
    {
        std::cout << "error to initialize\n";
        return 1;
    }

    server.run(serverFile);

    return 0;
}
