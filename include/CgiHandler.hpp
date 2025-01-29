#pragma once

#include <string>
#include <unistd.h>
#include <iostream>
#include <sys/wait.h>
#include <regex>
#include <string>
#include <set>


class HttpResponse;

class CgiHandler
{

	private :

	int pid;
	int pidResult;
	int status;
	int fdPipe[2];
	std::string cgiOut;

	public:

	CgiHandler();
	~CgiHandler();

	CgiHandler(const CgiHandler&); // = default;
    CgiHandler& operator=(const CgiHandler&); // = default;

	void executeCGI(std::string scriptPath, std::string queryString, std::string body, int method, HttpResponse &response);
	bool waitpidCheck(HttpResponse &response);
	std::string getCgiOut() const;
};