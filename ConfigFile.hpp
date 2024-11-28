#ifndef CONFIGFILE_HPP
#define CONFIGFILE_HPP

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <cctype>
#include <map>
#include <cstring>
#include <regex>

class ConfigFile 
{
private:
    std::string port;
    std::string _fileName;
    std::string ip_server;
    std::string server_name;
    std::string max_body;
    std::string errorPage;
    std::vector<std::string> locations;

public:

        class parseError : public std::exception {
        public:
            virtual const char* what() const throw()
            {
                return ("parse config file error\n");
            }
    };

    ConfigFile(std::string file);
    ~ConfigFile();

    std::string trim(const std::string &str);
    bool parseServerParams();
    void printParam();
    int getPort();
    std::string getIpServer();
    std::string getServerName();
};

#endif