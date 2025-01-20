#include "../include/HttpResponse.hpp"
#include "../include/CgiHandler.hpp"

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
	{405, "Method Not Allowed"},
	{415, "Unsupported Media Type"},
	{500, "Internal Server Error"},
	{501, "Wrong Method"},
	{502, "Bad Gateway"},
	{505, "HTTP Version Not Supported"},
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

std::string HttpResponse::getReasonPhrase() const
{
	return m_reasonPhrase;
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
	auto it = m_statusMap.find(code);
	if (it != m_statusMap.end()) {
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

LocationConfig findKey(std::string key, int mainKey, ConfigFile &confile)
{
	std::map<int, std::map<std::string, LocationConfig>> locations;
	locations = confile.getServerConfig();

    auto mainIt = locations.find(mainKey);
    if (mainIt == locations.end()) {
        throw std::runtime_error("Main key not found");
    }
    std::map<std::string, LocationConfig> &mymap = mainIt->second;
    auto it = mymap.find(key);
	if (it != mymap.end())
        return it->second;
	//std::cout << "key is" << key << std::endl;
	//std::cout << mymap.begin()->second.root << std::endl;
	if (!mymap.empty()){
		mymap.begin()->second.root += key; 
		return mymap.begin()->second;
	}
    throw std::runtime_error("Key not found");
}

std::string convertMethod(int method) {
	switch (method) {
		case GET:
			return "GET";
		case POST:
			return "POST";
		case DELETE:
			return "DELETE";
		default:
			return "GET";
	}
}

std::string formPath(std::string_view target, LocationConfig &locationConfig) {
	std::string location;
	std::string targetStr;// (target);
	std::string path;
	size_t pos = target.find('/');
	if (pos == std::string::npos)
		location = "/";
	size_t pos2 = target.find('/', pos + 1);
	if (pos2 == std::string::npos)
	{
		location = "/";
		targetStr = (std::string)target;
	}
	else
	{
	location = (std::string)target.substr(pos, pos2 - pos);
	targetStr = (std::string)target.substr(pos2);
	}
	/*
	try {
	} catch (std::runtime_error &e) {
		response.setStatusCode(404);
		response.setMimeType(".html");
		response.setHeader("Server", confile.getServerName(serverIndex));
		response.setBody("Not found");
		std::cout << "Location not found" << std::endl;
		return "";
	}
	if (locationConfig.limit_except.find(method) == std::string::npos) {
		response.setStatusCode(405);
		response.setMimeType(".html");
		response.setHeader("Server", confile.getServerName(serverIndex));
		response.setBody("Method Not Allowed");
		std::cout << "Method not allowed" << std::endl;
		return "";
	}*/
	path = "." + locationConfig.root + targetStr;
	return path;
}

std::string condenceLocation(const std::string_view &input){
	size_t pos = input.find('/');
	if (pos == std::string::npos)
		return "/";
	size_t pos2 = input.find('/', pos + 1);
	if (pos2 == std::string::npos)
		return "/";
	return (std::string)input.substr(pos, pos2 - pos);
}

bool	validateFile(std::string path, HttpResponse &response, LocationConfig &config, int serverIndex, int method, ConfigFile &confile) {
	// Check if the file exists
	if (!std::filesystem::exists(path)) {
		response.setStatusCode(404);
		response.setMimeType(".html");
		response.setHeader("Server", config.root);
		response.setBody("File not found");
		std::cout << "File not found" << std::endl;
		return false;
	}

	// Check file status
	struct stat filestat;
	if (stat(path.c_str(), &filestat) == -1) {
		response.setStatusCode(500);
		response.setMimeType(".html");
		response.setHeader("Server", config.root);
		response.setBody("Internal Server Error");
		std::cerr << "Error: Unable to stat file " << path << std::endl;
		return false;
	}
	if (config.limit_except.find(convertMethod(method)) == std::string::npos) {
		response.setStatusCode(405);
		response.setMimeType(".html");
		response.setHeader("Server", confile.getServerName(serverIndex));
		response.setBody("Method Not Allowed");
		std::cout << "Method not allowed" << std::endl;
		return false;
	}
	return true;
}

void handleDelete(HttpParser& request, ConfigFile &confile, int serverIndex, HttpResponse &response) {
	LocationConfig locationConfig = findKey(condenceLocation(request.getTarget()), serverIndex, confile);
	std::string path = formPath(request.getTarget(), locationConfig);
//	std::cout << "path is " << path << std::endl;
	if (response.getStatus() == 404 || response.getStatus() == 405)
		return;
	
	if (!validateFile(path, response, locationConfig, serverIndex, DELETE, confile))
		return;

    // Attempt to remove the file
    std::error_code ec;
    if (std::filesystem::remove(path, ec)) {
        response.setStatusCode(200);
        response.setMimeType(".txt");
        response.setHeader("Server", confile.getServerName(serverIndex));
        response.setBody("File deleted");
        std::cout << "File deleted" << std::endl;
    } else {
        if (ec) {
            response.setStatusCode(403);
            response.setMimeType(".html");
            response.setHeader("Server", confile.getServerName(serverIndex));
            response.setBody("Permission denied");
            std::cerr << "Error: " << ec.message() << std::endl;
        } else {
            response.setStatusCode(500);
            response.setMimeType(".html");
            response.setHeader("Server", confile.getServerName(serverIndex));
            response.setBody("Internal Server Error");
            std::cerr << "Internal Server Error" << std::endl;
        }
    }
}

std::string getExtension(const std::string_view& url) {
	size_t pos = url.find_last_of('.');
	if (pos == std::string::npos)
		return ".html";
	return std::string(url.substr(pos));
}

std::pair<int, std::string> locateAndReadFile(HttpParser &request, std::string& mime, ConfigFile &confile, int serverIndex, HttpResponse &response) {
	LocationConfig location;
	std::string locationStr = condenceLocation(request.getTarget());
	try {
	location = findKey(locationStr, serverIndex, confile);
	}	catch (std::runtime_error &e) {
		mime = ".html";
		return {500, "Location not found"};
		}
	std::string path;       // = "." + location.root;
	path = formPath(request.getTarget(), location);
	std::string error = confile.getErrorPage(serverIndex);
	if (request.getTarget() == "/")
		path += location.index;
//	std::cout << "path is " << path << std::endl;
	if (!validateFile(path, response, location, serverIndex, GET, confile))
	{
		std::cout << "error is " << error << std::endl;
	//	path = "." + error;
		std::ifstream file("." + error, std::ios::binary);
		std::ostringstream buffer;
		buffer << file.rdbuf();
		std::string bufferstr = buffer.str();
		size_t codePos = bufferstr.find("{{error_code}}");
		if (codePos != std::string::npos) {
		bufferstr.replace(codePos, 14, std::to_string(response.getStatus())); // Replace {{error_code}}
    	}
		size_t messagePos = bufferstr.find("{{error_message}}");
    	if (messagePos != std::string::npos) {
        bufferstr.replace(messagePos, 17, response.getReasonPhrase()); // Replace {{error_message}}
    	}
		response.setBody(bufferstr);
		mime = ".html";
		return {response.getStatus(), response.getBody()};
	}
//	std::cout << "locationstr is " << locationStr << std::endl;
//	std::cout << "path is " << path << std::endl;
	if (locationStr == "/cgi-bin") {
		CgiHandler cgi;
		std::string body;
		try {
			body = cgi.executeCGI(path, request.getQuery(), "", GET, response);
			response.setStatusCode(200);
		}
		catch (std::runtime_error &e) {
			std::ifstream file("." + error, std::ios::binary);
			std::ostringstream buffer;
			buffer << file.rdbuf();
			mime = ".html";
			body = buffer.str();
		}
		return {response.getStatus(), body};
	}
	struct stat fileStat;
	mime = getExtension(path);
	if (stat(path.c_str(), &fileStat) == -1)
	{
		std::ifstream file("." + error, std::ios::binary);
		std::ostringstream buffer;
		buffer << file.rdbuf();
		mime = ".html";
		path = "." + error;
		return {404, buffer.str()};
	}
	if (S_ISDIR(fileStat.st_mode)) {
		if (location.autoindex) {
			std::ostringstream buffer;
			buffer << "<!DOCTYPE html>\n<html><head><title>Index of " << request.getTarget() << "</title></head><body><h1>Index of " << request.getTarget() << "</h1><hr><pre>";
			DIR *dir;
			struct dirent *ent;
			if ((dir = opendir(path.c_str())) != NULL) {
				while ((ent = readdir(dir)) != NULL) {
					buffer << "<a href=\"" << ent->d_name << "\">" << ent->d_name << "</a><br>";
				}
				closedir(dir);
			}
			buffer << "</pre><hr></body></html>";
			mime = ".html";
			return {200, buffer.str()};
		}
		else
		return {403, "Forbidden"};
	}

	std::ifstream file(path, std::ios::binary);
	if (!file)
		return {500, "Failed to open file"};

	std::ostringstream buffer;
	buffer << file.rdbuf();
	return {200, buffer.str()};
}

HttpResponse receiveRequest(HttpParser& request, ConfigFile &confile, int serverIndex) {
	unsigned int status = request.getStatus();
	if (status != 200)
	{
		HttpResponse response;
		response.setStatusCode(status);
		response.setMimeType(".html");
		response.setHeader("Server", confile.getServerName(0));
		response.setBody("Bad Request");
		return response;
	}
	int	method = request.getMethod();
	std::string	mime;
	std::pair<int, std::string> file;
	HttpResponse response;
	switch (method) {
		case DELETE:
			mime = ".html";
			response.setStatusCode(status);
			response.setMimeType(mime);
			response.setHeader("Server", confile.getServerName(serverIndex));
			handleDelete(request, confile, serverIndex, response);
		//	response.setBody("Not found");
			return response;
		case GET:
			mime = getExtension(request.getTarget());
			file = locateAndReadFile(request, mime, confile, serverIndex, response);
			response.setStatusCode(file.first);
			response.setMimeType(mime);
			response.setHeader("Server", confile.getServerName(serverIndex));
			response.setBody(file.second);
			return response;
		case POST:
			status = 201;
			mime = ".txt";
			response.setStatusCode(status);
			response.setMimeType(mime);
			response.setHeader("Server", confile.getServerName(serverIndex));
			response.setBody("Created");
			return response;
		default:
			status = 404;
			mime = ".html";
			response.setStatusCode(status);
			response.setMimeType(mime);
			response.setHeader("Server", confile.getServerName(serverIndex));
			response.setBody("Not found");
			return response;
	}
}
