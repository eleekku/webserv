#include "../include/HttpResponse.hpp"
#include "../include/CgiHandler.hpp"
#include "Constants.hpp"

/*To use the HttpResponse declare HttpResponse object and define it with receiveRequest funciton which takes HttpParser object.
Then call HttpParser member function generate which will return the response as string.*/

HttpResponse::HttpResponse() : m_sent(false), m_totalBytesSent(0) {}

HttpResponse::HttpResponse(int code, std::string& mime) : m_statusCode(code), m_sent(false), m_mime(mime) {
	auto it = HTTP_STATUS_MESSAGES.find(code);
	if (it != HTTP_STATUS_MESSAGES.end()) {
		m_reasonPhrase = it->second;
	}
	else
		m_reasonPhrase = "Unknown";
	m_totalBytesSent = 0;
}

HttpResponse::~HttpResponse() {
}

HttpResponse::HttpResponse(const HttpResponse& other) {
	*this = other;
}

HttpResponse& HttpResponse::operator=(const HttpResponse& other) {
	if (this == &other)
		return *this;
	m_statusCode = other.m_statusCode;
	m_reasonPhrase = other.m_reasonPhrase;
	m_headers = other.m_headers;
	m_body = other.m_body;
	m_mime = other.m_mime;
	m_sent = other.m_sent;
	cgi = other.cgi;
	m_totalBytesSent = other.m_totalBytesSent;
	m_errorpath = other.m_errorpath;
	m_responsestr = other.m_responsestr;
	return *this;
}

std::string HttpResponse::getCurrentDate() const
{
	char buffer[128];
	time_t now = time(nullptr);
	struct tm tm = *gmtime(&now);
	strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", &tm);
	return std::string(buffer);
}

std::string HttpResponse::getMimeType(const std::string& extension) const
{
	auto it = MIME_TYPES.find(extension);
	if (it != MIME_TYPES.end()) {
		return it->second;
	}
	return "text/plain";
}

std::string HttpResponse::getMimeKey() const
{
	return m_mime;
}

std::string HttpResponse::getReasonPhrase() const
{
	return m_reasonPhrase;
}

std::string HttpResponse::getCgiBody() const
{
	return cgi->getCgiOut();
}

void HttpResponse::setHeader(const std::string& key, const std::string& value)
{
	m_headers[key] = value;
}

void HttpResponse::setBody(const std::string& body)
{
	m_body = body;
}

std::string HttpResponse::getBody() const
{
	return m_body;
}

void HttpResponse::setStatusCode(int code)
{
	m_statusCode = code;
	auto it = HTTP_STATUS_MESSAGES.find(code);
	if (it != HTTP_STATUS_MESSAGES.end()) {
		m_reasonPhrase = it->second;
	}
	else
		m_reasonPhrase = "Unknown";
}

int HttpResponse::getStatus() {
	return m_statusCode;
}

void HttpResponse::setMimeType(const std::string& mime)
{
	m_mime = mime;
}

void HttpResponse::setErrorpath(std::string errorpath)
{
	m_errorpath = errorpath;
}

std::string HttpResponse::getErrorpath() const
{
	return m_errorpath;
}

void HttpResponse::createCgi() {
	if (!cgi)
		cgi.emplace();
}

void	HttpResponse::startCgi(std::string scriptPath, std::string queryString, std::string body, int method, HttpResponse &response) {
	if (cgi)
		cgi->executeCGI(scriptPath, queryString, body, method, response);
}

bool HttpResponse::enterCgiWaitpid() {
	if (cgi)
		return cgi->waitpidCheck(*this);
	return false;
}

void HttpResponse::generate() {

	std::ostringstream response;

	response << "HTTP/1.1 " << m_statusCode << " " << m_reasonPhrase << "\r\n";
	if (m_headers.find("Date") == m_headers.end()) {
		response << "Date: " << getCurrentDate() << "\r\n";
	}
	if (m_headers.find("Content-Type") == m_headers.end()) {
		response << "Content-Type: " << getMimeType(m_mime) << "\r\n";
	}
	if (m_headers.find("Content-Length") == m_headers.end()) {
		if (m_body.empty() && m_statusCode != 204) 
			m_body = getReasonPhrase();
		if (m_statusCode != 204)
		response << "Content-Length: " << m_body.size() << "\r\n";
    }

	for (const auto& [key, value] : m_headers) {
		response << key << ": " << value << "\r\n";
	}
	response << "\r\n";
	if (!m_body.empty())
		response << m_body;
	else if (m_statusCode != 204) {
		response << getReasonPhrase();
	}
	m_responsestr = response.str();
}

void HttpResponse::errorPage() {
	std::ifstream file("." + getErrorpath(), std::ios::binary);
	std::ostringstream buffer;
	buffer << file.rdbuf();
	std::string bufferstr = buffer.str();
	size_t codePos = bufferstr.find("{{error_code}}");
	if (codePos != std::string::npos) {
		bufferstr.replace(codePos, 14, std::to_string(getStatus()));
    }
	size_t messagePos = bufferstr.find("{{error_message}}");
    if (messagePos != std::string::npos) {
        bufferstr.replace(messagePos, 17, getReasonPhrase());
    }
	setBody(bufferstr);
	setMimeType(".html");
}

bool HttpResponse::sendResponse(int serverSocket, int i)
{
	(void)i;
    //struct epoll_event events[10];
	size_t bodySize = m_responsestr.size();
	int bufferSize = bodySize - m_totalBytesSent;
    //if (events[i].events & EPOLLOUT)
    //{
	try {
		if (cgi && cgidone == false)
		{
		if (!cgi->waitpidCheck(*this))
			return false;
		else {
			setBody(cgi->getCgiOut());
			std::cout << "body to send\n" << m_body << "\n";
		//	setHeader("Server", confile.getServerName(serverIndex));
			generate();
			bodySize = m_responsestr.size();
		}
		}
	}
	catch (std::exception &e) {
		errorPage();
		bodySize = m_responsestr.size();
	}

	if (bufferSize > MAX_SIZE_SEND)
		bufferSize = MAX_SIZE_SEND;
	ssize_t bytesSent = send(serverSocket, m_responsestr.c_str() + m_totalBytesSent, bufferSize, MSG_NOSIGNAL);
	//std::cout << "body\n" << m_responsestr.c_str() << "\nbytesSent\n" << bytesSent << "\n";
	std::cout <<  "\n bytes to send\n" <<bytesSent << "\n";
	if (bytesSent == -1 || bytesSent == 0) //if this happen we need to created a new response (this mean a new body size)
	{
		std::cout << "Send fail to response client " << serverSocket << "\n";
	}
	else
		m_totalBytesSent += bytesSent;
	if (m_totalBytesSent < bodySize)
	{
		//readytoanswer[serverSocket] = true;
		return false;
	}
    //}
	m_sent = true;
	return true;
}

int	HttpResponse::getchildid()
{
	return cgi->getchildid();
}
