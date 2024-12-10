#include "../include/HttpResponse.hpp"

/*To use the HttpResponse declare HttpResponse object and define it with receiveRequest funciton which takes HttpParser object.
Then call HttpParser member function generate which will return the response as string.*/

const std::map<int, std::string> HttpResponse::m_statusMap = {
	{200, "OK"},
	{201, "Created"},
	{202, "Accepted"},
	{204, "No Content"},
	{400, "Bad Request"},
	{401, "Unauthorized"},
	{403, "Forbidden"},
	{404, "Not Found"},
	{415, "Unsupported Media Type"},
	{500, "Internal Server Error"},
	{502, "Bad Gateway"}
};

const std::map<std::string, std::string> HttpResponse::m_mimeTypes = {
	{".html", "text/html"},
	{".css", "text/css"},
	{".js", "application/javascript"},
	{".json", "application/json"},
	{".png", "image/png"},
	{".jpg", "image/jpeg"},
	{".jpeg", "image/jpeg"},
	{".gif", "image/gif"},
	{".txt", "text/plain"},
	{".pdf", "application/pdf"}
};

HttpResponse::HttpResponse() {}

HttpResponse::HttpResponse(int code, std::string& mime) : m_statusCode(code), m_sent(false), m_mime(mime) {
	auto it = m_statusMap.find(code);
	if (it != m_statusMap.end()) {
		m_reasonPhrase = it->second;
	}
	else 
		m_reasonPhrase = "Unknown";
}

HttpResponse::~HttpResponse() {}

HttpResponse::HttpResponse(const HttpResponse& other) {
	m_statusCode = other.m_statusCode;
	m_reasonPhrase = other.m_reasonPhrase;
	m_headers = other.m_headers;
	m_body = other.m_body;
	m_mime = other.m_mime;
	m_sent = other.m_sent;
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
	auto it = m_mimeTypes.find(extension);
	if (it != m_mimeTypes.end()) {
		return it->second;
	}
	return "text/plain";
}

std::string HttpResponse::getMimeKey() const
{
	return m_mime;
}

void HttpResponse::setHeader(const std::string& key, const std::string& value)
{
	m_headers[key] = value;
}

void HttpResponse::setBody(const std::string& body)
{
	m_body = body;
}

void HttpResponse::setStatusCode(int code)
{
	m_statusCode = code;
	auto it = m_statusMap.find(code);
	if (it != m_statusMap.end()) {
		m_reasonPhrase = it->second;
	}
	else 
		m_reasonPhrase = "Unknown";
}

void HttpResponse::setMimeType(const std::string& mime)
{
	m_mime = mime;
}

std::string HttpResponse::generate() const {

	std::ostringstream response;

	response << "HTTP/1.1 " << m_statusCode << " " << m_reasonPhrase << "\r\n";
	if (m_headers.find("Date") == m_headers.end()) {
		response << "Date: " << getCurrentDate() << "\r\n";
	}
	if (m_headers.find("Content-Type") == m_headers.end()) {
		response << "Content-Type: " << getMimeType(m_mime) << "\r\n";
	}

	if (m_headers.find("Content-Length") == m_headers.end()) {
        response << "Content-Length: " << m_body.size() << "\r\n";
    }

	for (const auto& [key, value] : m_headers) {
		response << key << ": " << value << "\r\n";
	}
	response << "\r\n";
	response << m_body;
	return response.str();
}

std::string getExtension(const std::string_view& url) {
	size_t pos = url.find_last_of('.');
	if (pos == std::string::npos)
		return ".html";
	return std::string(url.substr(pos));
}

std::pair<int, std::string> locateAndReadFile(std::string_view url, std::string& mime, ConfigFile &confile) {
	LocationConfig location;
	
	location = confile.findKey("/", "server 0");
	std::string root = "." + location.root;
	std::string fullPath = root + (std::string)url;
	struct stat fileStat;
	mime = getExtension(url);
	if (stat(fullPath.c_str(), &fileStat) == -1)
	{
		std::ifstream file("." + confile.getErrorPage(0), std::ios::binary);
		std::cout << "error url is " << root + confile.getErrorPage(0) << std::endl;
		std::ostringstream buffer;
		buffer << file.rdbuf();
		mime = ".html";
		return {404, buffer.str()};
	}
	if (S_ISDIR(fileStat.st_mode))
		return {403, "Forbidden"};

	std::ifstream file(fullPath, std::ios::binary);
	if (!file)
		return {500, "Failed to open file"};
	
	std::ostringstream buffer;
	buffer << file.rdbuf();
	return {200, buffer.str()};
}

HttpResponse receiveRequest(HttpParser& request, ConfigFile &confile) {
	int	method = request.getMethod();
	std::string	mime;
	std::pair<int, std::string> file;
	HttpResponse response;
	int code;
	switch (method) {
		case DELETE:
			code = 404;
			mime = ".html";
			response.setStatusCode(code);
			response.setMimeType(mime);
			response.setHeader("Server", confile.getServerName(0));
			response.setBody("Not found");
			return response;
		case GET:
			mime = getExtension(request.getTarget());
			file = locateAndReadFile(request.getTarget(), mime, confile);
			response.setStatusCode(file.first);
			response.setMimeType(mime);
			response.setHeader("Server", confile.getServerName(0));
			response.setBody(file.second);
			return response;

		case POST:
			code = 404;
			mime = ".html";
			response.setStatusCode(code);
			response.setMimeType(mime);
			response.setHeader("Server", confile.getServerName(0));
			response.setBody("Not found");
			return response;
		default:
			code = 404;
			mime = ".html";
			response.setStatusCode(code);
			response.setMimeType(mime);
			response.setHeader("Server", confile.getServerName(0));
			response.setBody("Not found");
			return response;
	}
}