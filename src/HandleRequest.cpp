#include "../include/HandleRequest.hpp"

LocationConfig findKey(std::string key, int mainKey, ConfigFile &confile) {
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
	path = "." + locationConfig.root + targetStr;
	return path;
}

std::string condenceLocation(const std::string_view &input) {
	size_t pos = input.find('/');
	if (pos == std::string::npos)
		return "/";
	size_t pos2 = input.find('/', pos + 1);
	if (pos2 == std::string::npos)
		return "/";
	return (std::string)input.substr(pos, pos2 - pos);
}

bool	validateFile(std::string path, HttpResponse &response, LocationConfig &config, int serverIndex, int method, ConfigFile &confile) { //if the file exists and all is ok
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

std::pair<int, std::string> handleDelete(HttpParser& request, ConfigFile &confile, int serverIndex, HttpResponse &response) { //deletes a file
	LocationConfig locationConfig = findKey(condenceLocation(request.getTarget()), serverIndex, confile);
	std::string path = formPath(request.getTarget(), locationConfig);
//	std::cout << "path is " << path << std::endl;
	if (response.getStatus() == 404 || response.getStatus() == 405)
		return returnErrorPage(response);

	if (!validateFile(path, response, locationConfig, serverIndex, DELETE, confile))
	{
		response.setStatusCode(403);
		return returnErrorPage(response);
	}

    // Attempt to remove the file
    std::error_code ec;
    if (std::filesystem::remove(path, ec)) {
        response.setStatusCode(204);
        std::cout << "File deleted" << std::endl;
		return {response.getStatus(), ""};
    } else {
        if (ec) {
            response.setStatusCode(403);
            response.setMimeType(".html");
            response.setBody("Permission denied");
            std::cerr << "Error: " << ec.message() << std::endl;
			return (returnErrorPage(response));
        } else {
            response.setStatusCode(500);
            std::cerr << "Internal Server Error" << std::endl;
			return (returnErrorPage(response));
        }
    }
}

std::string getExtension(const std::string_view& url) {
	size_t pos = url.find_last_of('.');
	if (pos == std::string::npos)
		return ".html";
	return std::string(url.substr(pos));
}

std::pair<int, std::string> returnErrorPage(HttpResponse &response) { // error code must be set before calling this function
	std::ifstream file("." + response.getErrorpath(), std::ios::binary);
	std::ostringstream buffer;
	buffer << file.rdbuf();
	std::string bufferstr = buffer.str();
	size_t codePos = bufferstr.find("{{error_code}}");
	if (codePos != std::string::npos) {
		bufferstr.replace(codePos, 14, std::to_string(response.getStatus()));
    }
	size_t messagePos = bufferstr.find("{{error_message}}");
    if (messagePos != std::string::npos) {
        bufferstr.replace(messagePos, 17, response.getReasonPhrase());
    }
	response.setBody(bufferstr);
	response.setMimeType(".html");
	return {response.getStatus(), response.getBody()};
}

std::pair<int, std::string> listDirectory(HttpParser &request, std::string &path, LocationConfig &location, HttpResponse &response) {
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
		response.setMimeType(".html");
		return {200, buffer.str()};
	}
	else
	{
		response.setStatusCode(403);
		return {403, "Forbidden"};
	}
}

std::pair<int, std::string> locateAndReadFile(HttpParser &request, ConfigFile &confile, int serverIndex, HttpResponse &response) {
	LocationConfig location;
	std::string locationStr = condenceLocation(request.getTarget());
	try {
	location = findKey(locationStr, serverIndex, confile);
	}	catch (std::runtime_error &e) {
		response.setStatusCode(500);
		return (returnErrorPage(response));
		}
	std::string path;       // = "." + location.root;
	path = formPath(request.getTarget(), location);
	std::string error = confile.getErrorPage(serverIndex);
	if (request.getTarget() == "/")
		path += location.index;
//	std::cout << "path is " << path << std::endl;
	if (!validateFile(path, response, location, serverIndex, GET, confile))
		return (returnErrorPage(response));
//	std::cout << "locationstr is " << locationStr << std::endl;
//	std::cout << "path is " << path << std::endl;
	if (locationStr == "/cgi") { //calls the cgi executor
		std::cout << "its cgi! " << std::endl;
	//	CgiHandler cgi;
		response.createCgi();
	//	response.startCgi(path, request.getQuery(), "", GET);
		std::string body;
		try {
			body = response.startCgi(path, request.getQuery(), "", GET, response);
			response.setStatusCode(200);
		}
		catch (std::runtime_error &e) {
			return (returnErrorPage(response));
		}
		return {response.getStatus(), body};
	}
	struct stat fileStat;
	stat(path.c_str(), &fileStat);
	response.setMimeType(getExtension(path));
	if (S_ISDIR(fileStat.st_mode))
		return (listDirectory(request, path, location, response));
	std::ifstream file(path, std::ios::binary);
	if (!file){
		response.setStatusCode(500);
		return (returnErrorPage(response));
	}

	std::ostringstream buffer;
	buffer << file.rdbuf();
	return {200, buffer.str()};
}

void handlePost(HttpParser &request, ConfigFile &confile, int serverIndex, HttpResponse &response) {
	LocationConfig location;
	std::string locationStr = condenceLocation(request.getTarget());
	if (locationStr != "/cgi") {
		response.setStatusCode(400);
		response.setMimeType(".html");
		response.setHeader("Server", confile.getServerName(serverIndex));
		response.setBody("Bad Request");
		return;
	}
	location = findKey(locationStr, serverIndex, confile);
	response.createCgi();
//	CgiHandler cgi;
	std::string body = response.startCgi(location.root, request.getQuery(), request.getBody(), POST, response);
	response.setStatusCode(200);
	response.setBody(body);
	response.setMimeType(".txt");
}

HttpResponse receiveRequest(HttpParser& request, ConfigFile &confile, int serverIndex) {
	HttpResponse response;
	response.setErrorpath(confile.getErrorPage(serverIndex));
	unsigned int status = request.getStatus();
//	std::cout << "status is " << status << std::endl;
	if (status != 200 && status != 201)
	{
		std::cout << "status is ll" << status << std::endl;
		response.setStatusCode(status);
		response.setMimeType(".html");
		response.setHeader("Server", confile.getServerName(0));
//		response.setBody("Bad Request");
		return response;
	}
	int	method = request.getMethod();
	std::pair<int, std::string> file;
	switch (method) {
		case DELETE:
			response.setStatusCode(status);
			response.setHeader("Server", confile.getServerName(serverIndex));
			handleDelete(request, confile, serverIndex, response);
		//	response.setBody("Not found");
			return response;
		case GET:
			file = locateAndReadFile(request, confile, serverIndex, response);
			response.setStatusCode(file.first);
			response.setHeader("Server", confile.getServerName(serverIndex));
			response.setBody(file.second);
			return response;
		case POST:
			response.setHeader("Server", confile.getServerName(serverIndex));
			if (status == 201) {
				response.setStatusCode(status);
				response.setMimeType(".txt");
				response.setBody("Created");
			}
			else
				handlePost(request, confile, serverIndex, response);
			return response;
		default:
			status = 405;
			response.setStatusCode(status);
			response.setMimeType(".html");
			response.setHeader("Server", confile.getServerName(serverIndex));
		//	response.setBody("Not found");
			return response;
	}
}
