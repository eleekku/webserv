#include "../include/HttpResponse.hpp"
#include "../include/HttpParser.hpp"
#include "Constants.hpp"

CgiHandler::CgiHandler() : pid(0), pidResult(0), status(0), cgiOut("") { }

CgiHandler::~CgiHandler() {}

CgiHandler::CgiHandler(const CgiHandler& src) {
    *this = src;
}

CgiHandler& CgiHandler::operator=(const CgiHandler& src) {
    this->pid = src.pid;
    this->pidResult = src.pidResult;
    this->status = src.status;
    this->cgiOut = src.cgiOut;
    this->fdPipe[0] = src.fdPipe[0];
    this->fdPipe[1] = src.fdPipe[1];
    this->m_childid = src.m_childid;
    return *this;
}

int childid = 0;
int pipetoclose = 0;

void timeoutHandler(int signal)
{
    if (signal == SIGALRM)
    {
  //      std::cerr << "pipetoclose " << pipetoclose << "\n";
        //close(pipetoclose);
        kill(childid, SIGINT);
  //      std::cerr << "Timeout reached. Killing child process: " << childid << std::endl;
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

void CgiHandler::executeCGI(std::string scriptPath, HttpParser &request, HttpResponse &response)
{
    struct epoll_event event;
    
    if (scriptPath.empty())
        throw std::runtime_error("Empty scriptPath\n");

    if (pipe(fdPipe) == -1)
        throw std::runtime_error("Pipe creation failed\n");

    fcntl(fdPipe[0], F_SETFL, O_NONBLOCK);
    pipetoclose = fdPipe[1];
    event.events = EPOLLOUT;
    event.data.fd = fdPipe[0];
    epoll_ctl(response.getEpoll(), EPOLL_CTL_ADD, fdPipe[0], &event);

    pid = fork();
    childid = pid;
    m_childid = pid;

    if (pid == -1)
        throw std::runtime_error("Fork failed\n");

    if (pid == 0)  // Child process
    {
        if (request.getMethod() == POST)
        {
            int pipeWrite[2];
            if (pipe(pipeWrite) == -1)
                exit(1);
            fcntl(pipeWrite[1], F_SETFL, O_NONBLOCK);
            std::string body = request.getBody();

            if (write(pipeWrite[1], request.getBody().c_str(), request.getContentLength()) == -1)
                exit(1);
            std::string contentLengthStr = std::to_string(request.getContentLength());
            setenv("CONTENT_LENGTH", contentLengthStr.c_str(), 1);
            close(pipeWrite[1]);
            dup2(pipeWrite[0], STDIN_FILENO);
            close(pipeWrite[0]);
        }
        setenv("REQUEST_METHOD", request.getMethod() == GET ? "GET" : "POST", 1);
        setenv("QUERY_STRING", request.getQuery().c_str(), 1);
        
        freopen("/dev/null", "w", stderr);  // Redirect errors to avoid printing on terminal
        dup2(fdPipe[1], STDOUT_FILENO);
        close(fdPipe[1]);
        close(fdPipe[0]);
            struct sigaction sa;
        sa.sa_handler = timeoutHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;  // Prevent `epoll_wait` from failing with EINTR
        if (sigaction(SIGALRM, &sa, NULL) == -1)
            std::cerr << "Error setting signal handler\n";
  //      std::string line;
     //   std::getline(std::cin, line);
    //    std::cerr << "line is " << line << "\n";
        int executeTimeOut = 8;
        alarm(executeTimeOut);

        char *argv[] = {const_cast<char *>(scriptPath.c_str()), nullptr};
    //    std::string line;
    //    std::getline(std::cin, line);
    //    std::cerr << "line is " << line << "\n";
        execvp(scriptPath.c_str(), argv);
        exit(1);
    }
    //epoll_ctl(response.getEpoll(), EPOLL_CTL_DEL, fdPipe[0], &event);
    close(fdPipe[1]);
}


bool CgiHandler::waitpidCheck(HttpResponse &response)
{
    if (response.getCgiDone())
        return true;
    pidResult =  waitpid(pid, &status, WNOHANG);
//    std::cout << "pidResult is " << pidResult << "\n";
//   std::cout << errno << "\n";
//    std::cerr << "read fd is " << fdPipe[0] << "\n";
    if (pidResult == 0)
    {
        response.setStatusCode(102);
        return false;
      //  throw std::runtime_error("cgi is still running");
    }
    else if (WIFSIGNALED(status))
    {
        std::cerr << "script terminated by signal " << "\n";
        //close(fdPipe[0]);
        response.setStatusCode(504);
        response.errorPage();
     //   alarm(0);
        //throw std::runtime_error("script terminated by signal\n");
    }
    //alarm(0);
    else if (WIFEXITED(status)) 
    {
        int exitStatus = WEXITSTATUS(status);
        if (exitStatus == 0) {
     //       std::cout << "script executed\n";
            char buffer[BUFFER_SIZE + 1];
  //      std::cerr << "fdPipe[0] is " << fdPipe[0] << "\n";
            int bitesRead = read(fdPipe[0], buffer, BUFFER_SIZE);
            if (bitesRead == -1) {
                std::cerr << "Error reading from pipe\n";
                response.setStatusCode(502);
                response.errorPage();
            }
//        std::cout << "bites read is " << bitesRead << "\n";
            buffer[bitesRead] = '\0';
            cgiOut += buffer;
            response.setStatusCode(200);
            response.setBody(cgiOut);
//        std::cerr << "script executed\n";
//        std::cout << cgiOut << "\n";
            }
        else
        {
            std::cerr << "exit status is " << exitStatus << "\n";
            std::cerr << "script terminated with error\n";
            response.setStatusCode(502);
            response.errorPage();
        }
    } 
    else
    {
        std::cerr << "error with waitpid\n";
        response.setStatusCode(502);
        response.errorPage();
    }
    response.setCgiDone(true); 
    return true;
}

void CgiHandler::terminateCgi()
{
    if (childid > 0)
    {
        std::cerr << "Terminating CGI process: " << childid << "\n";
        kill(m_childid, SIGINT);
        close(fdPipe[0]);
        close(fdPipe[1]);
    }
}

std::string CgiHandler::getCgiOut() const { return cgiOut;}

int CgiHandler::getchildid() { return pid; }

int CgiHandler::getFdPipe(){ return fdPipe[0]; }
