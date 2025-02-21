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
	m_epoll = other.m_epoll;
	cgiFdtoSend = other.cgiFdtoSend;
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
	struct stat filestat;
	std::string path = "." + errorpath;
	if (errorpath.empty())
		m_errorpath = DEFAULT_ERROR_PATH;
	else if (!std::filesystem::exists(path))
		m_errorpath = DEFAULT_ERROR_PATH;
	else if (stat(path.c_str(), &filestat) == -1)
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

void HttpResponse::startCgi(std::string scriptPath, HttpParser &request, std::vector<int> &_clientActivity) {
	if (cgi)
		cgi->executeCGI(scriptPath, request, _clientActivity);
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
	setBody(bufferstr);
	setMimeType(".html");
}

bool HttpResponse::sendResponse(int serverSocket, std::vector<int> &clientActivity)
{
	if (cgi)
	{
		if (!cgi->waitpidCheck(*this)) {
			if (cgiFdtoSend == 0)
			{
				struct epoll_event event;
				cgiFdtoSend = serverSocket;
				event.events = EPOLLOUT;
				event.data.fd = getFdPipe();
				epoll_ctl(m_epoll, EPOLL_CTL_ADD, getFdPipe(), &event);
			}
			return false;
		}
		else if (!m_sent) {
			if (cgiFdtoSend != 0)
				serverSocket = cgiFdtoSend;
			else
			{
				if (getFdPipe() > 3)
				{
					close(getFdPipe());
					auto new_end = std::remove(clientActivity.begin(), clientActivity.end(), getFdPipe());
					clientActivity.erase(new_end, clientActivity.end());
				}
			}
			generate();
		}
		else
			return true;
	}
	int bufferSize = m_bodySize - m_totalBytesSent;
	if (bufferSize > MAX_SIZE_SEND)
		bufferSize = MAX_SIZE_SEND;
	ssize_t bytesSent = send(serverSocket, m_responsestr.c_str() + m_totalBytesSent, bufferSize, MSG_NOSIGNAL);
	if (bytesSent == -1)
	{
		std::cerr << "Error sending response\n";
		m_totalBytesSent = 0;
		setStatusCode(500);
		errorPage();
		generate();
		return false;
	}
	else if (bytesSent == 0)
	{
		if (cgi)
		{
			if (cgiFdtoSend > 3)
				close(cgiFdtoSend);
			setCgiDone(true);
		}
		return (true);
	}
	else
		m_totalBytesSent += bytesSent;
	if (m_totalBytesSent < m_bodySize)
		return false;
	if (cgi)
	{
		if (cgiFdtoSend > 3)
			close(cgiFdtoSend);
		setCgiDone(true);
	}
	m_sent = true;
	return true;
}
int HttpResponse::getCgiFdtoSend() { return cgiFdtoSend; }
