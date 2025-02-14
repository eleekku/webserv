#include "../include/HttpResponse.hpp"
#include "../include/CgiHandler.hpp"
#include "Constants.hpp"

/*To use the HttpResponse declare HttpResponse object and send it to receiverequest funciton which takes HttpParser object.
Then call HttpParser member function generate which will return the response as string.*/

HttpResponse::HttpResponse() : m_sent(false), m_totalBytesSent(0) {
	m_statusCode = 200;
	m_reasonPhrase = "OK";
	m_mime = "text/plain";
	m_epoll = 0;
	cgiFdtoSend = 0;
	m_reasonPhrase = "OK";
	m_cgidone = false;
}

HttpResponse::HttpResponse(int code, std::string& mime) : m_statusCode(code), m_sent(false), m_mime(mime) {
	auto it = HTTP_STATUS_MESSAGES.find(code);
	if (it != HTTP_STATUS_MESSAGES.end()) {
		m_reasonPhrase = it->second;
	}
	else
		m_reasonPhrase = "Unknown";
	m_totalBytesSent = 0;
	m_epoll = 0;
	cgiFdtoSend = 0;
}

HttpResponse::~HttpResponse() {}

HttpResponse::HttpResponse(const HttpResponse& other) {
	*this = other;
}

HttpResponse& HttpResponse::operator=(const HttpResponse& other) {
//	std::cout << "hello from copy assignment\n";
	if (this == &other)
		return *this;
//	std::cout << "hello from copy assignment\n";
	m_statusCode = other.m_statusCode;
//	std::cout << "status code\n";
//	std::cout << "this reaason phrase is " << m_reasonPhrase << "\n";
//	std::cout << "other reason phrase is " << other.m_reasonPhrase << "\n";
	m_reasonPhrase = other.m_reasonPhrase;
//	std::cout << "reason Phrase\n";
	m_headers = other.m_headers;
//	std::cout << "headers\n";
	m_body = other.m_body;
//	std::cout << "body\n";
	m_mime = other.m_mime;
//	std::cout << "mime\n";
	m_sent = other.m_sent;
//	std::cout << "sent\n";
	cgi = other.cgi;
//	std::cout << "cgi\n";
	m_totalBytesSent = other.m_totalBytesSent;
//	std::cout << "totalBytesSent\n";
//	std::cout << "other error path is " << other.m_errorpath << "\n";
	m_errorpath = other.m_errorpath;
//	std::cout << "errorpath\n";
	m_responsestr = other.m_responsestr;
//	std::cout << "responsestr\n";
	m_epoll = other.m_epoll;
//	std::cout << "epoll\n";
	cgiFdtoSend = other.cgiFdtoSend;
//	std::cout << "bye from copy assignment\n";
	m_cgidone = other.m_cgidone;

	return *this;
}

// SETTERS

void HttpResponse::setHeader(const std::string& key, const std::string& value) { m_headers[key] = value; }

void HttpResponse::setBody(const std::string& body) { m_body = body; }

void HttpResponse::setStatusCode(int code) {
	m_statusCode = code;
	auto it = HTTP_STATUS_MESSAGES.find(code);
	if (it != HTTP_STATUS_MESSAGES.end()) {
		m_reasonPhrase = it->second;
	}
	else
		m_reasonPhrase = "Unknown";
}

void HttpResponse::setMimeType(const std::string& mime) { m_mime = mime; }

void HttpResponse::setErrorpath(std::string errorpath) {
	if (errorpath.empty())
		m_errorpath = DEFAULT_ERROR_PATH;
	std::string path = "." + errorpath;
	if (!std::filesystem::exists(path))
		m_errorpath = DEFAULT_ERROR_PATH;
	struct stat filestat;
	if (stat(path.c_str(), &filestat) == -1)
		m_errorpath = DEFAULT_ERROR_PATH;
	else
		m_errorpath = errorpath;
}

void HttpResponse::setEpoll(int epoll) { m_epoll = epoll; }

void HttpResponse::setCgiDone(bool cgidone) { m_cgidone = cgidone; }

void HttpResponse::setCgiFdtoSend(int fd) { cgiFdtoSend = fd; }

// GETTERS

std::string HttpResponse::getCurrentDate() const {
	char buffer[128];
	time_t now = time(nullptr);
	struct tm tm = *gmtime(&now);
	strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", &tm);
	return std::string(buffer);
}

std::string HttpResponse::getMimeType(const std::string& extension) const {
	auto it = MIME_TYPES.find(extension);
	if (it != MIME_TYPES.end()) {
		return it->second;
	}
	return "text/plain";
}

std::string HttpResponse::getMimeKey() const { return m_mime; }

std::string HttpResponse::getReasonPhrase() const { return m_reasonPhrase; }

std::string HttpResponse::getCgiBody() const { return cgi->getCgiOut(); }

std::string HttpResponse::getBody() const { return m_body; }

int	HttpResponse::getchildid() { return cgi->getchildid(); }

int HttpResponse::getFdPipe() { return cgi->getFdPipe(); }

int HttpResponse::getStatus() { return m_statusCode; }

std::string HttpResponse::getErrorpath() const { return m_errorpath; }

int HttpResponse::getEpoll() { return m_epoll; }

bool HttpResponse::getCgiDone() { return m_cgidone; }

// METHODS

void HttpResponse::createCgi() {
	if (!cgi)
		cgi.emplace();
}

void HttpResponse::startCgi(std::string scriptPath, HttpParser &request, HttpResponse &response) {
	if (cgi)
		cgi->executeCGI(scriptPath, request, response);
}

bool HttpResponse::checkCgiStatus() {
	if (cgi)
		return true;
	return false;
}

void HttpResponse::terminateCgi() {
	if (cgi)
		cgi->terminateCgi();
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
	m_bodySize = m_responsestr.size();
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
	std::cout << "error page bufferstr: " << bufferstr << std::endl;
	setBody(bufferstr);
	setMimeType(".html");
}

bool HttpResponse::sendResponse(int serverSocket)
{
	if (cgi)
	{
		std::cout << "checking cgi status\n";
		if (!cgi->waitpidCheck(*this)) {
			std::cout << "cgi not done\n";
	//		close(cgiFdtoSend);
			if (cgiFdtoSend == 0)
			{
				std::cout << "cgiFdtoSend set" << cgiFdtoSend << "\n";
				cgiFdtoSend = serverSocket;
			}
			return false;
		}
		else if (!m_sent) {
			std::cout << "cgi done not sent\n";
//			if (cgiFdtoSend != 0)
				serverSocket = cgiFdtoSend;
			generate();
	//		serverSocket = cgiFdtoSend;
		}
		else
			return true;
	}
	int bufferSize = m_bodySize - m_totalBytesSent;
	if (bufferSize > MAX_SIZE_SEND)
		bufferSize = MAX_SIZE_SEND;
	std::cerr << "sending response to " << serverSocket << "\n";
	ssize_t bytesSent = send(serverSocket, m_responsestr.c_str() + m_totalBytesSent, bufferSize, MSG_NOSIGNAL);
	if (bytesSent == -1) // || bytesSent == 0) //if this happen we need to created a new response (this mean a new body size)
	{
		std::cerr << "Error sending response\n";
		m_totalBytesSent = 0;
		setStatusCode(500);
		errorPage();
		generate();
		return false;
	}
	else
		m_totalBytesSent += bytesSent;
	if (m_totalBytesSent < m_bodySize)
		return false;
	if (cgi)
	{
		close(cgiFdtoSend);
		setCgiDone(true);
	}
	std::cout << "sent\n";
	m_sent = true;
	return true;
}
int HttpResponse::getCgiFdtoSend() { return cgiFdtoSend; }
