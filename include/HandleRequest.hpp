#pragma once

#include "HttpParser.hpp"
#include "ConfigFile.hpp"
#include "HttpResponse.hpp"
#include "CgiHandler.hpp"

class HttpResponse;

LocationConfig	findKey(std::string key, int mainKey, ConfigFile &confile);
std::string		convertMethod(int method);
std::string		formPath(std::string_view target, LocationConfig &locationConfig);
std::string		formPath(std::string_view target, LocationConfig &locationConfig);
std::string		condenceLocation(const std::string_view &input);
bool			validateFile(std::string path, HttpResponse &response, LocationConfig &config, int method);
void			handleDelete(HttpParser& request, ConfigFile &confile, int serverIndex, HttpResponse &response);
std::string		getExtension(const std::string_view& url);
void			locateAndReadFile(HttpParser &request, ConfigFile &confile, int serverIndex, HttpResponse &response);
void 			handlePost(HttpParser &request, ConfigFile &confile, int serverIndex, HttpResponse &response);
void			receiveRequest(HttpParser& request, ConfigFile &confile, int serverIndex, HttpResponse &response);
