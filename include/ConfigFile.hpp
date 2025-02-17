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
#include <set>

struct LocationConfig {
    std::string limit_except;
    std::string root;
    bool autoindex;
    std::string index;
};

class ConfigFile 
{
private:
    std::vector<int>                                        port;
    std::string                                             _fileName;
    std::vector<std::string>                                ip_server;
    std::vector<std::string>                                server_name;
    std::vector<std::string>                                max_body;
    std::vector<std::string>                                errorPage;
    std::vector<std::string>                                locations;
    std::vector<int>                                        indexLocations;
    std::map<int, std::map<std::string, LocationConfig>>    serverConfig;
    int                                                     openBracket;
    int                                                     closeBracket;
    bool                                                    insideServerBlock;
    unsigned long                                           indexServer;

public:

        class parseError : public std::exception {
        public:
            virtual const char* what() const throw()
            {
                return ("parse config file error\n");
            }
    };

    ConfigFile();
    ConfigFile(std::string file);
    ~ConfigFile();
    ConfigFile& operator=(const ConfigFile& other);

    //Utilities
    void                                                        finalCheck();
    std::string                                                 trim(const std::string &str);
    void                                                        CheckLocationKey(int indexServer, std::string newKey);
    bool                                                        parseServerParams(std::ifstream& file, unsigned long i);
    void                                                        printParam();
    const std::map<int, std::map<std::string, LocationConfig>>  getServerConfig() const;
    std::vector<std::string>                                    splitIntoLines(const std::string& str);
    void                                                        openConfigFile();
    int                                                         serverAmount();
    void                                                        firstStepParsingCF(std::ifstream& file, std::string& line);
    void                                                        secondStepParsingCF(std::ifstream& file, std::string& line, int indexSer);

    //Getters
    std::vector<int>                                            getPort();
    std::vector<std::string>                                    getIpServer();
    std::string                                                 getServerName(int i);
    std::string                                                 getErrorPage(int i);
    long                                                        getMax_body(int i);

    //Setters
    std::vector<std::string>                                    setLocationBlock(int indexServer);
    void                                                        setLocations(int i);
};

#endif