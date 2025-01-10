#pragma once

#include <string>
#include <map>
#include <ctime>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <sys/stat.h>
#include <dirent.h>
#include "HttpParser.hpp"
#include "ConfigFile.hpp"
#include "Server.hpp"

class HttpResponse
{
public:
	HttpResponse();
	HttpResponse(int code, std::string& mime);
	~HttpResponse();
	HttpResponse(const HttpResponse& other);
	HttpResponse& operator=(const HttpResponse& other);

	void setHeader(const std::string& key, const std::string& value);
	void setBody(const std::string& body);
	void setStatusCode(int code);
	void setMimeType(const std::string& mime);
	std::string generate() const;

	void	setStatus(bool status);
	int		getStatus();
	std::string getMimeType(const std::string& extension) const;
	std::string getMimeKey() const;



private:

	int			m_statusCode;
	bool		m_sent;
	std::map	<std::string, std::string> m_headers;
	std::string	m_reasonPhrase;
	std::string	m_body;
	std::string m_mime;
	std::string	getCurrentDate() const;
	static const std::map<int, std::string> m_statusMap;
	static const std::map<std::string, std::string> m_mimeTypes;
};

std::pair<int, std::string> locateAndReadFile(std::string_view url, std::string& mime, ConfigFile &confile);
std::string getExtension(const std::string_view& url);

LocationConfig findKey(std::string key, std::string mainKey, ConfigFile &confile);
HttpResponse	receiveRequest(HttpParser& request, ConfigFile &confile, int serverIndex);

