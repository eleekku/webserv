#pragma once

#include <string>
#include <unistd.h>
#include <iostream>
#include <sys/wait.h>
#include <regex>
#include <string>
#include <set>
#include "Server.hpp"

//class Server;
class HttpResponse;

class CgiHandler
{

	private :

	int			pid;
	int			pidResult;
	int			status;
	int			fdPipe[2];
	int			m_childid;
	std::string	cgiOut;

	public:

	CgiHandler();
	~CgiHandler();

	CgiHandler(const CgiHandler&);
    CgiHandler& operator=(const CgiHandler&);

	void executeCGI(std::string scriptPath, HttpParser &request, std::vector<int> &_clientActivity);
	bool waitpidCheck(HttpResponse &response);
	void terminateCgi();

	// GETTERS

	std::string	getCgiOut() const;
	int			getchildid();
	int			getFdPipe();
};