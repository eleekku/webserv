#include "../include/CgiHandler.hpp"
#include "../include/HttpResponse.hpp"

CgiHandler::CgiHandler() {}

CgiHandler::~CgiHandler() {}

void timeoutHandler(int signal) 
{
    if (signal == SIGALRM)
    {
        throw std::runtime_error("Timeout executing script.");
    }
}

std::set<std::string> pythonKeywords = {
    "False", "None", "True", "and", "as", "assert", "async", "await", "break",
    "class", "continue", "def", "del", "elif", "else", "except", "finally",
    "for", "from", "global", "if", "import", "in", "is", "lambda", "nonlocal",
    "not", "or", "pass", "raise", "return", "try", "while", "with", "yield"
};

bool isValidPythonFilename(std::string& filename) 
{

    std::regex pattern(R"(^[a-zA-Z_][a-zA-Z0-9_]*\.py$)");

    if (!std::regex_match(filename, pattern)) 
        return false;

    std::string nameWithoutExtension = filename.substr(0, filename.size() - 3);

    if (pythonKeywords.find(nameWithoutExtension) != pythonKeywords.end()) 
        return false;

    return true;
}

std::string getPythonName(std::string& path) 
{
    size_t pos = path.find_last_of("/");

    if (pos == std::string::npos) {
        return path;
    }
    return path.substr(pos + 1);
}

std::string CgiHandler::executeCGI(std::string scriptPath, std::string queryString, std::string body, int method)
{
    if (scriptPath.size() == 0)
     return NULL;
    std::string strtest = getPythonName(scriptPath);
    if(!isValidPythonFilename(strtest))
        throw std::runtime_error("invalid file name\n");;
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

        //avoid to show the error in the terminal;
        freopen("/dev/null", "w", stderr);

        dup2(fdPipe[1], STDOUT_FILENO);
        close(fdPipe[1]);
        close(fdPipe[0]);

        char *argv[] = {const_cast<char *> (scriptPath.c_str()), 0};
        execvp(scriptPath.c_str(), argv);
        throw std::runtime_error("execvp fail\n");
    }
    else
    {
        int executeTimeOut = 5;
        close(fdPipe[1]);
        signal(SIGALRM, timeoutHandler);
        alarm(executeTimeOut);
        char buffer[1000];
        std::string strOut = "";
        int bitesRead;
        while ((bitesRead = read(fdPipe[0], buffer, sizeof(buffer) - 1)) > 0)
        {
            buffer[bitesRead] = '\0';
            strOut += buffer;
        }
        close(fdPipe[0]);
        int status;
        waitpid(pid, &status, 0);
        alarm(0);
        if (WIFEXITED(status) && !WEXITSTATUS(status))
        {
            std::cout << "script executed\n";
            std::cout << strOut << "\n";
            return strOut;
        } 
        else
            throw std::runtime_error("script can not execute\n");;
    }
}