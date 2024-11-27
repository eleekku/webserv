#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include "ConfigFile.hpp" 

class ConfigFile;

class Server {
private:
    int port;
    int serveSocket;
    std::string ipServer;
    std::string serverName;

public:

    Server(int portServer, std::string ipserver, std::string servername);
    ~Server();
    bool initialize();
    void run();

};

#endif