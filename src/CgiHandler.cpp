#include "../include/CgiHandler.hpp"

CgiHandler::CgiHandler() {}

CgiHandler::~CgiHandler() {}

std::string CgiHandler::executeCgi(const std::string& path)
{
	std::string output;
    FILE* pipe = popen(path.c_str(), "r");
    if (!pipe) {
        return "Status: 500 Internal Server Error\r\n\r\n";
    }

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }

    pclose(pipe);
    return output;
}