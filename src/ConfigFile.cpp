#include "../include/ConfigFile.hpp"


//  clean code 
//  vector location need to parsing 

void ConfigFile::printParam() //remenber to delete when project is done
{
    for(size_t i = 0;  i < port.size(); i++)
    {
        std::cout << "IP Server: " << ip_server[i] << "\n"
                << "Server Name: " << server_name[i] << "\n"
                << "Port: " << port[i] << "\n"
                << "Max Body Size: " << max_body[i] << "\n"
                << "Error Page: " << errorPage[i] << "\n";
    }
}

ConfigFile::ConfigFile(std::string file) : _fileName(file), openBracket(0), closeBracket(0), indexServer(0) {}

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

static bool isValIp(const std::string& ip) 
{    
    std::regex ipv4Regex(    R"(^(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9]?[0-9])(\.(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9]?[0-9])){3}$)"    );
    return std::regex_match(ip, ipv4Regex);
}
static bool isValPort(const std::string& port)
{
    std::regex serverPort( "^(0|[1-9][0-9]{0,3}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])$");
    return std::regex_match(port, serverPort);
}
static bool isValBZ(const std::string& bodySize)
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

void ConfigFile::firstStepParsingCF(std::ifstream& file, std::string& line) //it will show a error if it does not find "server" and "{",  
{
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
                {
                    insideServerBlock = true;
                }
            }
        }
        else 
            throw std::runtime_error("\nError invalid conf file");
        if (insideServerBlock == true)
            break;
    }
}

void ConfigFile::secondStepParsingCF(std::ifstream& file, std::string& line, int indexSer)
{
    bool flagListen = true;
    bool flagServerName = true;
    bool flagCMBZ = true;
    bool flagErrorP = true;
    std::string currentLocation;

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
    indexLocations.push_back(indexL);
}

void ConfigFile::finalCheck()
{
    if (ip_server.size() !=  indexServer
        || max_body.size() != indexServer
        || server_name.size() != indexServer
        || errorPage.size() != indexServer
        || port.size() != indexServer )
    {
        throw std::runtime_error("\nServer error final check\n");
    }
    indexServer--;
    for (unsigned long  i = 0; i < indexServer; i++ )
    {
        for (unsigned long  j = i; j < indexServer; j++)
        {
            if (ip_server[i] == ip_server[j + 1] && port[i] == port[j + 1])
                throw std::runtime_error("\n ip or port are same en diferents servers\n");
        }
    }
}

bool ConfigFile::parseServerParams(std::ifstream& file, unsigned long indexSer) 
{
    std::string line;

    indexServer += 1;

    firstStepParsingCF(file, line);

    secondStepParsingCF(file, line, indexSer);
    
    if (insideServerBlock == true)
        throw parseError();
    if (line.find("}") != std::string::npos)
        setLocations(indexSer);
    if (ip_server.empty()
        || max_body.empty()
        || server_name.empty()
        || errorPage.empty()) 
    {
        throw std::runtime_error("\nserver error");
    }
    indexSer += 1;
    if (std::getline(file, line))
    {
        parseServerParams(file, indexSer);
    }
    return true;
}

std::vector<std::string> ConfigFile::splitIntoLines(const std::string& str) 
{
    std::istringstream stream(str);
    std::vector<std::string> lines;
    std::string line;

    while (std::getline(stream, line)) 
        lines.push_back(trim(line));

    return lines;
}

static bool stringToBool(const std::string& str) { return str == "on" || str == "true"; }

static bool isValidLimit(const std::string& example) //maybe check if there are methodo duplicates
{
    std::regex validPattern(R"((POST|GET|DELETE)(\s(POST|GET|DELETE))*$)");
    return std::regex_match(example, validPattern);
}

void ConfigFile::CheckLocationKey(int indexServer, std::string newKey)
{
    const auto& innerMap = serverConfig[indexServer];

    for (const auto& innerPair : innerMap) 
    {
        if (innerPair.first == newKey)
            throw std::runtime_error("\nDuplicate Location Key\n");
    }
}

std::vector<std::string> ConfigFile::setLocationBlock(int indexServer)
{
    int i;
    std::vector<std::string> locationBlock;

    if (indexServer == 0)
        i = 0;
    else
        i = indexLocations[indexServer - 1];
    int end = i;
    while (i < indexLocations[indexServer] + end)
    {
        locationBlock.push_back(locations[i]);
        i++;
    }
    return locationBlock;
}

void ConfigFile::setLocations(int indexServer) //set a duoble map with the locations for each server
{
    std::string currentLocation;
    LocationConfig config;
    std::vector<std::string> lines;
    std::vector<std::string> locationBlock;

    locationBlock = setLocationBlock(indexServer);

    if (locations.empty() || locations[indexServer].empty())
        throw std::runtime_error("\n\nfile conf error in location part\n");

    serverConfig[indexServer] = std::map<std::string, LocationConfig>();

    for (const std::string& locationConfig : locationBlock) 
    {
        lines = splitIntoLines(locationConfig);

        for (const auto& line : lines) 
        {
            if (line.starts_with("location")) 
            {
                size_t start = line.find("location") + 8;
                size_t end = line.find("{");
                if (end != std::string::npos)
                    openBracket++;
                currentLocation = trim(line.substr(start, end - start));
            }
            else if (line.starts_with("{"))
                    openBracket++;
            else if (line.starts_with("limit_except")) 
            {
                size_t start = line.find("limit_except") + 12;
                size_t end = line.find(";");
                if (end == std::string::npos)
                    throw std::runtime_error("\n\nError locations\n\n");
                config.limit_except = trim(line.substr(start, end - start));
                if(!isValidLimit(config.limit_except))
                    throw std::runtime_error("\nError no valid limit_except\n");
            } 
            else if (line.starts_with("root")) 
            {
                size_t start = line.find("root") + 4;
                size_t end = line.find(";");
                if (end == std::string::npos)
                    throw std::runtime_error("\n\nError locations\n\n");
                config.root = trim(line.substr(start, end - start));
            } 
            else if (line.starts_with("autoindex")) 
            {
                size_t start = line.find("autoindex") + 9;
                size_t end = line.find(";");
                if (end == std::string::npos)
                    throw std::runtime_error("\n\nError locations\n\n");
                config.autoindex = stringToBool(trim(line.substr(start, end - start)));
            } 
            else if (line.starts_with("index")) 
            {
                size_t start = line.find("index") + 5;
                size_t end = line.find(";");
                if (end == std::string::npos)
                    throw std::runtime_error("\n\nError locations\n\n");
                config.index = trim(line.substr(start, end - start));
            }
            else if (line.starts_with("}"))
                closeBracket++;
        }
        CheckLocationKey(indexServer, currentLocation);
        serverConfig[indexServer][currentLocation] = config;
    }
    if (openBracket != closeBracket)
        throw std::runtime_error("\n\nunclosed bracket Locations\n");
    
}

bool isValidMaxBody(const std::string& arg) {
    std::regex pattern(R"(^\d+[MK]$)");
    return std::regex_match(arg, pattern);
}
//getters
const std::map<int, std::map<std::string, LocationConfig>> ConfigFile::getServerConfig() const {   return serverConfig; }
std::string ConfigFile::getErrorPage(int i) { return errorPage[i]; }
long ConfigFile::getMax_body(int i) 
{
    size_t end = 0;
    long body = 0;
    std::string number = "";
    if (!isValidMaxBody(max_body[i]))
    {
        throw std::runtime_error("\ninvalid MAX_BODY");
    }
    if ((end = max_body[i].find("M")) != std::string::npos)
    {
        number = max_body[i].substr(0, end);
        if (!(body = std::stoi(number)))
        {
            throw std::runtime_error("\nMAXBODY too big");
        }
        if (body > 10)
            throw std::runtime_error("\nMAXBODY too big");
        return(std::stoi(number) * 1000000);
    }
    if ((end = max_body[i].find("K")) != std::string::npos)
    {
        number = max_body[i].substr(0, end);
        if (!(body = std::stoi(number)))
        {
            throw std::runtime_error("\nMAXBODY too big");
        }
        if (body > 10000)
        {
            throw std::runtime_error("\nMAXBODY too big");
        }
        return(std::stoi(number) * 1000);
    }
    return body;
}
int ConfigFile::serverAmount() {  return port.size(); }
std::vector<int> ConfigFile::getPort() { return port; }
std::vector<std::string> ConfigFile::getIpServer() { return ip_server; }
std::string ConfigFile::getServerName(int i) { return server_name[i]; }