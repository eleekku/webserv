#include "ConfigFile.hpp"

void ConfigFile::printParam()
{
    std::cout << "IP Server: " << ip_server << "\n"
              << "Server Name: " << server_name << "\n"
              << "Port: " << port << "\n"
              << "Max Body Size: " << max_body << "\n"
              << "Error Page: " << errorPage << "\n";

    for (const std::string& locations : locations) {
        std::cout << locations << " \n";
    }

}

ConfigFile::ConfigFile(std::string file) : _fileName(file), port(""), ip_server(""), server_name(""), max_body(""), errorPage(""){}

ConfigFile::~ConfigFile() {}

std::string ConfigFile::trim(const std::string &str) 
{
    size_t start = str.find_first_not_of(" \t");
    size_t end = str.find_last_not_of(" \t");
    if (start == std::string::npos || end == std::string::npos) {
        return "";
    }
    return str.substr(start, end - start + 1);
}

bool isValIp(const std::string& ip) 
{
    
    std::regex ipv4Regex(    R"(^(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9]?[0-9])(\.(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9]?[0-9])){3}$)"    );
    return std::regex_match(ip, ipv4Regex);
}
bool isValPort(const std::string& port)
{
    std::regex serverPort( "^(0|[1-9][0-9]{0,3}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])$");
    return std::regex_match(port, serverPort);
}
bool isValBZ(const std::string& bodySize)
{
    std::regex maxBodySize ("^(10|[1-9](?:[.,][0-9])?)M$");
    return std::regex_match(bodySize, maxBodySize);
}

bool ConfigFile::parseServerParams()
{
    std::string line;
    std::string currentLocation;
    bool insideServerBlock = true;
    bool flagListen = true;
    bool flagServerName = true;
    bool flagCMBZ = true;
    bool flagErrorP = true;
    bool isBlockC = false;

    std::ifstream file(_fileName);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << _fileName << std::endl;
        return false;
    }

    while(std::getline(file, line)) 
    {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        if (line.find ("server") == 0)
        {
            size_t spacePos = line.find(' ');
            if (spacePos == std::string::npos)
            {
                while (std::getline(file, line))
                {
                    line = trim(line);
                    if (line.empty() || line[0] == '#') {
                    continue;
                    }
                    if (line.find("{") == 0)
                        break;
                }
            }
            else
            {
                if (line.find("{") != std::string::npos)
                {
                    break;
                }
            }
        }
    }
    while (std::getline(file, line)) 
    {
        
        line = trim(line);
        // Skip empty lines or comment lines
        if (line.empty() || line[0] == '#') {
            continue;
        }
        if (line.find("listen") == 0 && flagListen) 
        {
            flagListen = false;
            size_t spacePos = line.find(' ');
            if (spacePos != std::string::npos) {
                std::string listenValue = trim(line.substr(spacePos + 1));
                size_t colonPos = listenValue.find(':');
                if (colonPos != std::string::npos) 
                {
                    ip_server = listenValue.substr(0, colonPos);
                    if (!isValIp(ip_server))
                        return false;
                    size_t semiCo = listenValue.find(';');
                    if (semiCo != std::string::npos)
                    {
                        port = listenValue.substr(colonPos + 1, semiCo - colonPos -1);
                        if (!isValPort(port))
                            return false;
                    }
                }
                else 
                {
                    std::cerr << "Error: Invalid listen directive format." << std::endl;
                    return false;
                }
            }
        }

        else if (line.find("server_name") == 0 && flagServerName) 
        {
            flagServerName = false;
            size_t spacePos = line.find(' ');
            if (spacePos != std::string::npos) {
                size_t semiCo = line.find(';');
                if (semiCo != std::string::npos)
                    server_name = line.substr(spacePos + 1, semiCo - spacePos - 1);
                ///maybe check error here;
            }
        }

        else if (line.find("client_max_body_size") == 0 && flagCMBZ) 
        {
            flagCMBZ = false;
            size_t spacePos = line.find(' ');
            if (spacePos != std::string::npos) 
            {
                size_t semiCo = line.find(';');
                if (semiCo != std::string::npos)
                    max_body = line.substr(spacePos + 1, semiCo - spacePos - 1);
                if (!isValBZ(max_body))
                    return false;
            }
        }

        else if (line.find("error_page") == 0 && flagErrorP) 
        {
            flagErrorP = false;
            size_t spacePos = line.find(' ');
            if (spacePos != std::string::npos) 
            {
                size_t semiCo = line.find(';');
                if (semiCo != std::string::npos)
                    errorPage = line.substr(spacePos + 1, semiCo - spacePos - 1);
            }
        }

        else if (line.find("location") == 0) 
        {
            size_t spacePos = line.find(' ');
            if (spacePos != std::string::npos) 
                currentLocation = line.substr(spacePos + 1);

            std::string temp;
            temp = line + "\n";
            while (std::getline(file, line)) 
            {
                line = trim(line);
                if (line.empty() || line[0] == '#') 
                    continue;
                temp += line + "\n"; 
                if (line.find("}") != std::string::npos) 
                {
                    locations.push_back (temp);
                    temp.clear();
                    break; // End of location block
                }
            }
        }
        else if (line.find("}") != std::string::npos)
        {
            break ;
        }
        else
            return false;
    }
    return true;
}

int ConfigFile::getPort() { return (std::stoi(port)); }
std::string ConfigFile::getIpServer() { return ip_server; }
std::string ConfigFile::getServerName() { return server_name; }

