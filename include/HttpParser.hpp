#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <map>
#include <sstream>
#include <vector>

#define BUFFER_SIZE 8192

enum e_http_method {
	DELETE = 0,
	GET = 1,
	POST = 2,
	UNKNOWN = 3
};

class HttpParser
{
	private:
		// Variables
		std::vector<char>					_request;
		size_t								_pos;
		uint8_t								_method_enum;
		int									_status;
		std::string							_method;
		std::string							_target;
		std::string							_version;
		std::map<std::string, std::string>	_headers;
		std::map<std::string, std::string>	_bodyHeaders;
		std::string							_boundary;
		std::string							_query;
		size_t								_contentLength;
		std::map<std::string, std::string>	_formFields;

		// Parsing
		std::stringstream	getVectorLine();
		std::vector<char>	getBodyData();
		void				extractReqLine();
		void				parseQuery();
		void				extractHeaders(bool body);

		// Body processing
		void								readBody(int serverSocket);
		void								extractChunkedBody();
		void								extractBody();
		void								extractBoundary();
		void								extractContentLength();
		void								extractMultipartFormData();

		// Checking functions
	public:
		// Constructor
		HttpParser();

		// Getters
		std::map<std::string, std::string>	getHeaders();
		std::string							getMethodString();
		std::string							getTarget();
		std::string							getQuery();
		uint8_t								getMethod();
		int									getStatus();

		void	startParsing(std::vector<char>& request, int serverSocket);
};
