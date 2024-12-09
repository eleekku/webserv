#include "../include/ConfigFile.hpp"

void ConfigFile::printParam()
{
    for(size_t i = 0;  i < port.size(); i++)
    {
        std::cout << "IP Server: " << ip_server[i] << "\n"
                << "Server Name: " << server_name[i] << "\n"
                << "Port: " << port[i] << "\n"
                << "Max Body Size: " << max_body[i] << "\n"
                << "Error Page: " << errorPage[i] << "\n"
                << "index location: " << indexLocation[i] << "\n";
    }
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

bool ConfigFile::parseServerParams(std::ifstream& file, int indexSer) //remember have to split 
{
    std::string line;
    std::string currentLocation;
    insideServerBlock = false;
    bool flagListen = true;
    bool flagServerName = true;
    bool flagCMBZ = true;
    bool flagErrorP = true;

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
                    if (!isValIp(ip_server[indexSer]))
                        throw parseError();
                    size_t semiCo = listenValue.find(';');
                    if (semiCo != std::string::npos)
                    {
                        std::string portstr = listenValue.substr(colonPos + 1, semiCo - colonPos -1);
                        //port.push_back(listenValue.substr(colonPos + 1, semiCo - colonPos -1));
                        if (!isValPort(portstr))
                            throw parseError();
                        port.push_back(std::stoi(portstr));
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
                if (!isValBZ(max_body[indexSer]))
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
            break ;
        }
        else
            throw parseError();;
    }
    if (insideServerBlock == true)
        throw parseError();
    indexLocation.push_back(indexL);
    if (line.find("}") != std::string::npos)
        setLocations(indexSer);
    indexSer += 1;
    if (std::getline(file, line))
        parseServerParams(file, indexSer); // continuar trabajando aqui
    return true;
}

std::vector<int> ConfigFile::getPort() { return port; }
std::vector<std::string> ConfigFile::getIpServer() { return ip_server; }
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

void ConfigFile::setLocations(int i) //chequea si alguna key se repite debo mostrar un error
{
    std::string currentLocation;
    LocationConfig config;
    std::vector<std::string> lines;
    std::string serverkey = "server " + std::to_string(i);

    serverConfig[serverkey] = std::map<std::string, LocationConfig>();

    for (const std::string& locationConfig : locations) 
    {
        lines = splitIntoLines(locationConfig);

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

        serverConfig[serverkey][currentLocation] = config;
    }
}



const std::map<std::string, std::map<std::string, LocationConfig>>& ConfigFile::getServerConfig() const 
{
    return serverConfig;
}


std::vector<int> ConfigFile::getIndexLocation()
{
    return indexLocation;
}

std::string ConfigFile::getErrorPage(int i) { return errorPage[i]; }
std::string ConfigFile::getMax_body(int i) { return max_body[i]; }
int ConfigFile::serverAmoung()
{
    return port.size();
}

/*LocationConfig &ConfigFile::findKey(std::string key, std::string mainKey) 
{
    std::map<std::string, std::map<std::string, LocationConfig>> locations;
    locations = getServerConfig();
    std::map<std::string, LocationConfig> mymap = locations.find(mainKey);
    auto it2 = it.find(key);
    if (it2 != correctmap.end())
        return it->second;
    throw std::runtime_error("Key not found");
}*/

