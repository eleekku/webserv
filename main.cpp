#include <iostream>
#include "HttpResponse.hpp"

int main() {
    std::string url = "hel.he.lo.txt.txt";
    std::string mime = ".html";
    std::pair<int, std::string> file = locateAndReadFile(url, mime);
    //std::cout << "content is " << file.second << std::endl;
    HttpResponse response(file.first, mime);
    response.setHeader("Server", "Webserv/1.0");
    response.setBody(file.second);
    std::cout << response.generate();



    /*
    std::string mime = "text/plain";
    HttpResponse response(404, mime);
    response.setHeader("Server", "Webserv/1.0");
    response.setBody("Error 404: Page not found");

    std::cout << response.generate();
    */
    return 0;
}