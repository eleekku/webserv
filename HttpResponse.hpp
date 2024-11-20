#pragma once

#include <string>
#include <map>
#include <ctime>
#include <sstream>

class HttpResponse
{
public:
	HttpResponse(int code, const std::string& reason);
	~HttpResponse();

	void setHeader(const std::string& key, const std::string& value);
	void setBody(const std::string& body);
	std::string generate() const;

private:

	int			m_statusCode;
	std::map	<std::string, std::string> m_headers;
	std::string	m_reasonPhrase;
	std::string	m_body;
	std::string	getCurrentDate() const;
};