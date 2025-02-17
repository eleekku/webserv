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
        kill(childid, SIGINT);
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
        // SET ENVIRONMENT VARIABLES
        setenv("REQUEST_METHOD", request.getMethod() == GET ? "GET" : "POST", 1);
        setenv("QUERY_STRING", request.getQuery().c_str(), 1);
        freopen("/dev/null", "w", stderr);  // Redirect errors to avoid printing on terminal
        dup2(fdPipe[1], STDOUT_FILENO);
        close(fdPipe[1]);
        close(fdPipe[0]);
        struct sigaction sa; // Set signal handler for timeout
        sa.sa_handler = timeoutHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;  // Prevent `epoll_wait` from failing with EINTR
        if (sigaction(SIGALRM, &sa, NULL) == -1)
            std::cerr << "Error setting signal handler\n";
        int executeTimeOut = 8;
        alarm(executeTimeOut);
        char *argv[] = {const_cast<char *>(scriptPath.c_str()), nullptr};
        execvp(scriptPath.c_str(), argv);
        exit(1);
    }
    close(fdPipe[1]);
}

bool CgiHandler::waitpidCheck(HttpResponse &response)
{
    if (response.getCgiDone())
        return true;
    pidResult =  waitpid(pid, &status, WNOHANG);
    if (pidResult == 0)
    {
        response.setStatusCode(102);
        return false;
    }
    else if (WIFSIGNALED(status))
    {
        std::cerr << "script terminated by signal " << "\n";
        response.setStatusCode(504);
        response.errorPage();
    }
    else if (WIFEXITED(status)) 
    {
        int exitStatus = WEXITSTATUS(status);
        if (exitStatus == 0) {
            char buffer[BUFFER_SIZE + 1];
            int bitesRead = read(fdPipe[0], buffer, BUFFER_SIZE);
            if (bitesRead == -1) {
                std::cerr << "Error reading from pipe\n";
                response.setStatusCode(502);
                response.errorPage();
            }
            buffer[bitesRead] = '\0';
            cgiOut += buffer;
            response.setStatusCode(200);
            response.setBody(cgiOut);
            }
        else
        {
            std::cerr << "script exited with status " << exitStatus << "\n";
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
        kill(m_childid, SIGINT);
        close(fdPipe[0]);
        close(fdPipe[1]);
    }
}

// GETTERS

std::string CgiHandler::getCgiOut() const { return cgiOut;}

int CgiHandler::getchildid() { return pid; }

int CgiHandler::getFdPipe(){ return fdPipe[0]; }
