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
#include <sstream>

struct LocationConfig {
    std::string limit_except;
    std::string root;
    bool autoindex;
    std::string index;
};

class ConfigFile 
{
private:
    std::vector<int> port;
    std::string _fileName;
    std::vector<std::string> ip_server;
    std::vector<std::string> server_name;
    std::vector<std::string> max_body;
    std::vector<std::string> errorPage;
    std::vector<std::string> locations;
    std::map<std::string, std::map<std::string, LocationConfig>> serverConfig;
    bool insideServerBlock;
    std::vector<int> indexLocation;

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
    bool parseServerParams(std::ifstream& file, int i);
    void printParam();
    std::vector<int> getPort();
    std::vector<std::string> getIpServer();
    std::string getServerName(int i);
    std::string getErrorPage(int i);
    std::string getMax_body(int i);
    const std::map<std::string, std::map<std::string, LocationConfig>> getServerConfig() const;
    void setLocations(int i);
    std::vector<std::string> splitIntoLines(const std::string& str);
    void openConfigFile();
    int serverAmoung();
    std::vector<int> getIndexLocation();
};

#endif