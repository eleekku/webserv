#pragma once
#include "ConfigFile.hpp"
#include "Constants.hpp"
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <string>
#include <unordered_map>
#include <sstream>
#include <vector>

typedef std::unordered_map<std::string, std::string> map_t;

class HttpParser
{
	private:
		// Variables
		ParsingState		_state;
		std::vector<char>	_request;
		std::vector<char>	_tmp;
		size_t				_pos;
		size_t				_totalBytesRead;
		uint8_t				_method_enum;
		int					_status;
		std::string			_method;
		std::string			_target;
		std::string			_version;
		map_t				_headers;
		map_t				_bodyHeaders;
		std::string			_body;
		size_t				_maxBodySize;
		std::string			_boundary;
		std::string			_query;
		size_t				_contentLength;
		time_t				_lastSeen;
		bool				_keepAlive;
		std::string			_uploadFolder;

		// Parsing
		void				readRequest(int clientfd);
		void				readBody(int clientfd);
		std::stringstream	getVectorLine();
		std::vector<char>	getBodyData();
		void				extractReqLine();
		void				parseQuery();
		void				extractHeaders(bool body);

		// Body processing
		void		extractChunkedBody();
		void		extractBody();
		void		extractBoundary();
		void		extractContentLength();
		void		extractMultipartFormData();
		void		extractOctetStream();
		void		extractStringBody();
		std::string	extractFilename();
		std::string getUploadPath(ConfigFile& conf, int serverIndex);

		// Checking functions
		void	checkReadRequest();
		bool	checkRequest(std::stringstream& line);
		void	checkHeaders(std::string_view key, std::string_view value);
		void	checkLimitMethods(ConfigFile& conf, int serverIndex);
		bool	checkValidCharacters();
		void	isKeepAlive();

	public:
		// Constructor
		HttpParser();

		// Getters
		map_t		getHeaders();
		std::string	getMethodString();
		std::string	getTarget();
		std::string	getQuery();
		std::string getBody();
		uint8_t		getMethod();
		int			getStatus();
		size_t		getContentLength();
		bool		checkTimeout();
		bool		getKeepAlive();

		bool	startParsing(int clientfd, ConfigFile& conf, int serverIndex);
};
