#pragma once

#include "CgiHandler.hpp"
#include <string>
#include <map>
#include <ctime>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <sys/stat.h>
#include <dirent.h>
#include <optional>
#include "HttpParser.hpp"
#include "ConfigFile.hpp"
#include "Server.hpp"
#include "HandleRequest.hpp"

static const int MAX_SIZE_SEND = 20000000;

class HttpResponse
{
	public:
		HttpResponse();
		HttpResponse(int code, std::string& mime);
		~HttpResponse();
		HttpResponse(const HttpResponse& other);
		HttpResponse& operator=(const HttpResponse& other);

		void 		setHeader(const std::string& key, const std::string& value);
		void 		setBody(const std::string& body);
		void 		setStatusCode(int code);
		void 		setMimeType(const std::string& mime);
		void 		setErrorpath(std::string errorpath);
		void		setEpoll(int epoll);

		std::string	getBody() const;
		int			getStatus();
		std::string getMimeType(const std::string& extension) const;
		std::string getMimeKey() const;
		std::string getReasonPhrase() const;
		std::string getErrorpath() const;
		std::string getCgiBody() const;
		int			getchildid();
		int 		getFdPipe();
		int 		getEpoll();

		void		createCgi();
		void		startCgi(std::string scriptPath, HttpParser &request, HttpResponse &response);
		void		generate();
		bool		sendResponse(int serverSocket);
		void 		errorPage();
		bool		checkCgiStatus();
		int			getCgiFdtoSend();

	private:

		int 											cgiFdtoSend;
		std::optional<CgiHandler> 						cgi;
		int												m_epoll;
		int												m_statusCode;
		bool											m_sent;
		size_t											m_totalBytesSent;
		size_t											m_bodySize;
		std::map<std::string, std::string>				m_headers;
		std::string										m_reasonPhrase;
		std::string										m_body;
		std::string 									m_mime;
		std::string 									m_errorpath;
		std::string 									m_responsestr;
		std::string										getCurrentDate() const;
		std::string										m_defaulterrorpath;
		static const std::map<int, std::string>			m_statusMap;
		static const std::map<std::string, std::string>	m_mimeTypes;
};
