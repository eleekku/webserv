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

bool validateFile(std::string path, HttpResponse &response, LocationConfig &config, int method) {
	// Check if the file exists
	if (!std::filesystem::exists(path)) {
		response.setStatusCode(404);
		response.errorPage();
		return false;
	}
	// Check if the file is a symbolic link
	if (std::filesystem::is_symlink(path)) {
		response.setStatusCode(403);
		response.errorPage();
		std::cerr << "Forbidden: Symbolic link not allowed" << std::endl;
		return false;
	}
	// Check file status
	struct stat filestat;
	if (stat(path.c_str(), &filestat) == -1) {
		if (errno == EACCES)
			response.setStatusCode(403);
		else
			response.setStatusCode(500);
		response.errorPage();
		std::cerr << "Error: Unable to stat file " << path << std::endl;
		return false;
	}
	if (config.limit_except.find(convertMethod(method)) == std::string::npos) {
		response.setStatusCode(405);
		std::cerr << "Method not allowed" << std::endl;
		response.errorPage();
		return false;
	}
	return true;
}

void handleDelete(HttpParser& request, ConfigFile &confile, int serverIndex, HttpResponse &response) { //deletes a file
	LocationConfig locationConfig;
	try {
		locationConfig = findKey(condenceLocation(request.getTarget()), serverIndex, confile);
	}
	catch (const std::runtime_error& e) {
		response.setStatusCode(500);
		response.errorPage();
		return;	
	}
	std::string path = formPath(request.getTarget(), locationConfig);
	if (response.getStatus() == 404 || response.getStatus() == 405) {
		response.errorPage();
		return;
	}
	if (!validateFile(path, response, locationConfig, DELETE))
		return;
    // Attempt to remove the file
    std::error_code ec;
    if (std::filesystem::remove(path, ec)) {
        response.setStatusCode(204);
		return;
    } else {
        if (ec) {
            response.setStatusCode(403);
            std::cerr << "Error: " << ec.message() << std::endl;
			response.errorPage();
			return;
        } else {
            response.setStatusCode(500);
            std::cerr << "Internal Server Error" << std::endl;
			response.errorPage();
			return;
        }
    }
}

std::string getExtension(const std::string_view& url) {
	size_t pos = url.find_last_of('.');
	if (pos == std::string::npos)
		return ".html";
	return std::string(url.substr(pos));
}

void listDirectory(HttpParser &request, std::string &path, LocationConfig &location, HttpResponse &response) {
	if (location.autoindex) {
		std::ostringstream buffer;
		buffer << "<!DOCTYPE html>\n<html><head><title>Index of " << request.getTarget() << "</title></head><body><h1>Index of " << request.getTarget() << "</h1><hr><pre>";
		DIR *dir;
		struct dirent *ent;
		if ((dir = opendir(path.c_str())) != NULL) {
			while ((ent = readdir(dir)) != NULL) {
				buffer << "<a href=\"" << request.getTarget() << "/" << ent->d_name << "\">" << ent->d_name << "</a><br>";
			}
			closedir(dir);
		}
		buffer << "</pre><hr></body></html>";
		response.setBody(buffer.str());
		response.setMimeType(".html");
		return;
	}
	else
	{
		response.setStatusCode(403);
		return;
	}
}

void locateAndReadFile(HttpParser &request, ConfigFile &confile, int serverIndex, HttpResponse &response, std::vector<int> &clientActivity) {
	LocationConfig location;
	std::string locationStr = condenceLocation(request.getTarget());
	try {
	location = findKey(locationStr, serverIndex, confile);
	}	catch (std::runtime_error &e) {
		response.setStatusCode(500);
		response.errorPage();
		return;
		}
	std::string path;
	path = formPath(request.getTarget(), location);
	std::string error = confile.getErrorPage(serverIndex);
	if (request.getTarget() == "/")
		path += location.index;
	if (!validateFile(path, response, location, GET))
		return;
	if (locationStr == "/cgi") { //calls the cgi executor
		response.createCgi();
		try {
			response.startCgi(path, request, clientActivity);
			response.setStatusCode(102);
			return;
		}
		catch (std::runtime_error &e) {
			std::cerr << "parent catched\n";
			response.errorPage();
			return;
		}

	}
	struct stat fileStat;
	stat(path.c_str(), &fileStat);
	response.setMimeType(getExtension(path));
	if (S_ISDIR(fileStat.st_mode)) {
		if (path.back() != '/')
			path += "/";
		return (listDirectory(request, path, location, response));
	}
	std::ifstream file(path, std::ios::binary);
	if (!file){
		response.setStatusCode(500);
		response.errorPage();
		return;
	}
	std::ostringstream buffer;
	buffer << file.rdbuf();
	response.setBody(buffer.str());
	return;
}

void handlePost(HttpParser &request, ConfigFile &confile, int serverIndex, HttpResponse &response, std::vector<int> &clientActivity) {
	LocationConfig location;
	std::string locationStr = condenceLocation(request.getTarget());
	if (locationStr != "/cgi") { // other POSTS are handled in the parsing
		response.setStatusCode(400);
		response.errorPage();
		return;
	}
	try {
	location = findKey(locationStr, serverIndex, confile);
	}
	catch (std::runtime_error &e) {
		response.setStatusCode(500);
		response.errorPage();
		return;
	}
	std::string path = formPath(request.getTarget(), location);
	if (request.getTarget() == "/")
		path += location.index;
	response.createCgi();
	response.startCgi(path, request, clientActivity);
		response.setStatusCode(102);
		return;
}

void receiveRequest(HttpParser& request, ConfigFile &confile, int serverIndex, HttpResponse &response, std::vector<int> &clientActivity) {
	response.setHeader("Server", confile.getServerName(serverIndex));
	response.setErrorpath(confile.getErrorPage(serverIndex));
	unsigned int status = request.getStatus();
	response.setStatusCode(status);
	if (status != 200 && status != 201 && status != 204)
	{
		response.setStatusCode(status);
		response.errorPage();
		return;
	}
	switch (request.getMethod()) {
		case DELETE:
			response.setStatusCode(status);
			handleDelete(request, confile, serverIndex, response);
			return;
		case GET:
			locateAndReadFile(request, confile, serverIndex, response, clientActivity);
			return;
		case POST:
			response.setHeader("Server", confile.getServerName(serverIndex));
			if (status == 201) {
				response.setStatusCode(status);
				response.setMimeType(".txt");
				response.setBody("Created");
			}
			else
				handlePost(request, confile, serverIndex, response, clientActivity);
			return;
		default:
			response.setStatusCode(405);
			response.errorPage();
			return;
	}
}
