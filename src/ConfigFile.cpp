#include "../include/ConfigFile.hpp"

void ConfigFile::printParam()
{
    int i = -1;
    while (i > port.size())
    {
        i++;
        std::cout << "IP Server: " << ip_server[i] << "\n"
                << "Server Name: " << server_name[i] << "\n"
                << "Port: " << port[i] << "\n"
                << "Max Body Size: " << max_body[i] << "\n"
                << "Error Page: " << errorPage[i] << "\n";
    }
    /*for (const std::string& locations : locations) {
        std::cout << locations << " \n";
    }*/

}

ConfigFile::ConfigFile(std::string file) : _fileName(file), indexLocation(0) {}

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

void ConfigFile::openConfigFile()
{
    std::ifstream file(_fileName);
    if (!file.is_open()) 
    {
        throw parseError();
    }
    parseServerParams(file, 0);
}

bool ConfigFile::parseServerParams(std::ifstream& file, int i) //remember have to split 
{
    std::string line;
    std::string currentLocation;
    insideServerBlock = false;
    bool flagListen = true;
    bool flagServerName = true;
    bool flagCMBZ = true;
    bool flagErrorP = true;
    bool isBlockC = false;

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
                    {
                        insideServerBlock = true;
                        break;
                    }
                }
            }
            else
            {
                if (line.find("{") != std::string::npos)
                    insideServerBlock = true;
            }
        }
        if (insideServerBlock == true)
            break;
    }
    //if (!std::getline(file, line))
    //    throw parseError();
    int indexL = 0;
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
                    ip_server.push_back(listenValue.substr(0, colonPos));
                    if (!isValIp(ip_server[i]))
                        throw parseError();
                    size_t semiCo = listenValue.find(';');
                    if (semiCo != std::string::npos)
                    {
                        port.push_back(listenValue.substr(colonPos + 1, semiCo - colonPos -1));
                        if (!isValPort(port[i]))
                            throw parseError();;
                    }
                }
                else 
                {
                    throw parseError();;
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
                    server_name.push_back(line.substr(spacePos + 1, semiCo - spacePos - 1));
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
                    max_body.push_back(line.substr(spacePos + 1, semiCo - spacePos - 1));
                if (!isValBZ(max_body[i]))
                    throw parseError();
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
                    errorPage.push_back(line.substr(spacePos + 1, semiCo - spacePos - 1));
                //check error pag;
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
                    indexL++;
                    break; // End of location block
                }
            }
        }
        else if (line.find("}") != std::string::npos)
        {
            insideServerBlock = false;
            std::getline(file, line);
            break ;
        }
        else
            throw parseError();;
    }
    if (insideServerBlock == true)
        throw parseError();
    indexLocation.push_back(indexL);
    setLocations();
    std::cout << "estas es la ultima linea -----\n" << line << "\n";
    if (std::getline(file, line))
        parseServerParams(file, i++); // continuar trabajando aqui
    return true;
}

int ConfigFile::getPort(int i) { return (std::stoi(port[i])); }
std::string ConfigFile::getIpServer(int i) { return ip_server[i]; }
std::string ConfigFile::getServerName(int i) { return server_name[i]; }

std::vector<std::string> ConfigFile::splitIntoLines(const std::string& str) 
{
    std::istringstream stream(str);
    std::vector<std::string> lines;
    std::string line;

    while (std::getline(stream, line)) {
        lines.push_back(trim(line));
    }

    return lines;
}

bool stringToBool(const std::string& str) {
    return str == "on" || str == "true";
}

void ConfigFile::setLocations()
{
    std::string currentLocation;
    LocationConfig config;
    std::vector<std::string> lines;
    for (const std::string& locations : locations) 
    {
        lines = splitIntoLines(locations);

        for (const auto& line : lines) {
            if (line.starts_with("location")) {

                size_t start = line.find("location") + 8;
                size_t end = line.find("{");
                currentLocation = trim(line.substr(start, end - start));
            } else if (line.starts_with("limit_except")) {

                size_t start = line.find("limit_except") + 12;
                size_t end = line.find(";");
                config.limit_except = trim(line.substr(start, end - start));
            } else if (line.starts_with("root")) {

                size_t start = line.find("root") + 4;
                size_t end = line.find(";");
                config.root = trim(line.substr(start, end - start));
            } else if (line.starts_with("autoindex")) {

                size_t start = line.find("autoindex") + 9;
                size_t end = line.find(";");
                config.autoindex = stringToBool(trim(line.substr(start, end - start)));
            } else if (line.starts_with("index")) {

                size_t start = line.find("index") + 5;
                size_t end = line.find(";");
                config.index = trim(line.substr(start, end - start));
            }
        }

        serverConfig[currentLocation] = config;

    }
    // Imprimir la configuración
    for (const auto& [key, config] : serverConfig) {
        std::cout << key << ":\n";
        std::cout << "  limit_except: " << config.limit_except << "\n";
        std::cout << "  root: " << config.root << "\n";
        std::cout << "  autoindex: " << (config.autoindex ? "on" : "off") << "\n";
        std::cout << "  index: " << config.index << "\n";
    }
}

