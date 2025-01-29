#pragma once

#include <string>
#include <unistd.h>
#include <iostream>
#include <sys/wait.h>
#include <regex>
#include <string>
#include <set>
#include "HttpResponse.hpp"


class HttpResponse;

class CgiHandler
{

	private :

	int pid;
	int pidResult;
	int status;
	int flagChildProcess;
	int fdPipe[2];
	std::string strOut;

	public:

	CgiHandler();
	~CgiHandler();

	std::string executeCGI(std::string scriptPath, std::string queryString, std::string body, int method, HttpResponse &response);
};