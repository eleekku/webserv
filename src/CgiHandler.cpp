#include "../include/CgiHandler.hpp"
#include "../include/HttpResponse.hpp"

CgiHandler::CgiHandler() {}

CgiHandler::~CgiHandler() {}

std::string CgiHandler::executeCGI(std::string scriptPath, std::string queryString, std::string body, int method)
{
    if (scriptPath.size() == 0)
     return NULL;

    std::string strMethod = "";
    if (method == GET)
        strMethod = "GET";
    else if (method == POST)
        strMethod = "POST";


    int fdPipe[2];
    if (pipe(fdPipe) == -1)
    {
        throw std::runtime_error("Pipe fail\n");
    }

    int pid = fork();
    if (pid == -1)
    {
        throw std::runtime_error("fork fail\n");
    }

    if (pid == 0)
    {
        if (method == POST)
        {
            int pipeWrite[2];
            if(pipe(pipeWrite) == -1)
            {
                throw std::runtime_error("Pipe write fail\n");
            }
            write(pipeWrite[1], body.c_str(), body.size());
            close(pipeWrite[1]);
            dup2(pipeWrite[0], STDIN_FILENO);
            close(pipeWrite[0]);
        }
        setenv("REQUEST_METHOD", strMethod.c_str(), 0);
        setenv("QUERY_STRING", queryString.c_str(), 0);
        dup2(fdPipe[1], STDOUT_FILENO);
        close(fdPipe[1]);
        close(fdPipe[0]);

        char *argv[] = {const_cast<char *> (scriptPath.c_str()), 0};
        execvp(scriptPath.c_str(), argv);
        throw std::runtime_error("execvp fail\n");
    }
    else
    {
        close(fdPipe[1]);
        char buffer[1000];
        std::string strOut;
        int bitesRead;
        while ((bitesRead = read(fdPipe[0], buffer, sizeof(buffer) - 1)) > 0)
        {
            buffer[bitesRead] = '\0';
            strOut += buffer;
        }
        close(fdPipe[0]);
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status))
        {
            std::cout << "script executed\n";
            return strOut;
        }
        else
            return NULL;
    }
}