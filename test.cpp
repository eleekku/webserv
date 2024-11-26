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

#define PORT 8080

class HTTPRequest {
public:
    std::string method;
    std::string uri;
    std::string version;

    HTTPRequest(const std::string &requestText) {
        parse(requestText);
    }

    void parse(const std::string &requestText) {
        size_t methodEnd = requestText.find(' ');
        size_t uriEnd = requestText.find(' ', methodEnd + 1);
        method = requestText.substr(0, methodEnd);
        uri = requestText.substr(methodEnd + 1, uriEnd - methodEnd - 1);
        version = requestText.substr(uriEnd + 1, requestText.find('\n') - uriEnd - 1);
    }
};

class HTTPResponse {
public:
    int statusCode;
    std::string statusMessage;
    std::string body;

    HTTPResponse(int code, const std::string &message, const std::string &content) 
        : statusCode(code), statusMessage(message), body(content) {}

    std::string toString() {
        std::string response = "HTTP/1.1 " + std::to_string(statusCode) + " " + statusMessage + "\n";
        response += "Content-Type: text/html\n";
        response += "Content-Length: " + std::to_string(body.size()) + "\n\n";
        response += body;
        return response;
    }
};

class ConnectionHandler {
public:
    int clientSocket;

    explicit ConnectionHandler(int socket) : clientSocket(socket) {}

    std::string executeCGI(const std::string &scriptPath) {
        char buffer[1024];
        std::string output;

        // Crear un pipe para capturar la salida del script CGI
        FILE *pipe = popen(scriptPath.c_str(), "r");
        if (!pipe) {
            return "Error al ejecutar el script CGI";
        }

        // Leer la salida del script línea por línea
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output += buffer;
        }

        // Cerrar el pipe y devolver la salida
        pclose(pipe);
        return output;
    }

    void handle() {
        char buffer[30000] = {0};
        int bytesRead = read(clientSocket, buffer, 30000);

        if (bytesRead <= 0) {
            // Manejar caso de solicitud vacía o error de lectura
            HTTPResponse response(400, "Bad Request", "<html><body><h1>400 Bad Request</h1></body></html>");
            std::string responseText = response.toString();
            send(clientSocket, responseText.c_str(), responseText.size(), 0);
            close(clientSocket);
            return;
        }

        HTTPRequest request(buffer);
        std::cout << "Solicitud recibida: " << request.method << " " << request.uri << std::endl;

        if (request.uri == "/cgi-bin/script") {
            // Ejecutar un script CGI y capturar la salida
            std::string scriptOutput = executeCGI("./cgi-bin/script.py");

            HTTPResponse response(200, "OK", scriptOutput);
            std::string responseText = response.toString();
            send(clientSocket, responseText.c_str(), responseText.size(), 0);
        }
        else if (request.uri == "/") {
            // Responder con contenido por defecto
            HTTPResponse response(200, "OK", "<html><body><h1>Hello World</h1></body></html>");
            std::string responseText = response.toString();
            send(clientSocket, responseText.c_str(), responseText.size(), 0);
        }
        else {
            // Respuesta estándar para 404
            HTTPResponse response(404, "Not Found", "<html><body><h1>404 Not Found</h1></body></html>");
            std::string responseText = response.toString();
            send(clientSocket, responseText.c_str(), responseText.size(), 0);
        }

        close(clientSocket);
    }
};

class Server {
private:
    int serverSocket;
    struct sockaddr_in address;

public:
    Server() {
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
        setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
        
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(PORT);
        
        bind(serverSocket, (struct sockaddr*)&address, sizeof(address));
        listen(serverSocket, 3);
        
        std::cout << "Servidor iniciado en el puerto " << PORT << std::endl;
    }

    void run() {
        while (true) {
            int addrlen = sizeof(address);
            int clientSocket = accept(serverSocket, (struct sockaddr*)&address, (socklen_t*)&addrlen);
            if (clientSocket < 0) {
                std::cerr << "Error al aceptar la conexión" << std::endl;
                continue;
            }

            ConnectionHandler handler(clientSocket);
            handler.handle();
        }
    }

    ~Server() {
        close(serverSocket);
    }
};

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
    ConfigFile(std::string file);
    ~ConfigFile();

    bool readConfigFile();
    std::string trim(const std::string &str);
    bool parseServerParams();
    void printParam();
};

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

bool ConfigFile::readConfigFile()
{
    return true;
}

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

int main(int ac, char **av) 
{
    if (ac != 2) {
        std::cout << "Too many arguments" << std::endl;
        return -1;
    }

    // Create a ConfigFile instance with the provided file
    ConfigFile serverFile(av[1]);

    /*if (validateConfig(av[1]))
        std::cout << "valid config" << std::endl;
    else 
        std::cout << "Not valid config" << std::endl;*/
    // Parse the server parameters from the config file
    if (!serverFile.parseServerParams()) 
    {
        std::cerr << "Error parsing server parameters." << std::endl;
        return -1;
    }

    // Print the parameters to verify they were correctly parsed
    serverFile.printParam();
    //Server server;
    //server.run();
    return 0;
}
