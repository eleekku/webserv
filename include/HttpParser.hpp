#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <sstream>
#include <vector>
#include <array>

#define BUFFER_SIZE 8192

enum e_http_method {
	DELETE = 0,
	GET = 1,
	POST = 2,
	UNKNOWN = 3
};

typedef std::unordered_map<std::string, std::string> map_t;

class HttpParser
{
	private:
		// Variables
		std::vector<char>	_request;
		size_t				_pos;
		uint8_t				_method_enum;
		int					_status;
		std::string			_method;
		std::string			_target;
		std::string			_version;
		map_t				_headers;
		map_t				_bodyHeaders;
		std::string			_body;
		std::string			_boundary;
		std::string			_query;
		size_t				_contentLength;

		static constexpr uint8_t METHOD = 1 << 0;
		static constexpr uint8_t TARGET = 1 << 1;
		static constexpr uint8_t HEADER = 1 << 2;
		static constexpr uint8_t VALUE = 1 << 3;
		static constexpr uint8_t HEX = 1 << 4;
		static constexpr uint8_t DIGIT = 1 << 5;
		static constexpr uint8_t SEPARATOR = 1 << 6;
		static constexpr uint8_t WHITESPACE = 1 << 7;

		static constexpr std::array<uint8_t, 256> _TABLE = []()
				{
					std::array<uint8_t, 256> table = {};

					for (int i = 0; i < 256; i++)
					{
						unsigned char c = static_cast<unsigned char>(i);
						uint8_t& flags = table[i];
						if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
						{
							flags |= METHOD;
							flags |= TARGET;
							flags |= HEADER;
							flags |= VALUE;
							if ((c <= 'F') || (c >= 'a' && c <= 'f'))
								flags |= HEX;
						} else if ((c >= '0' && c <= '9'))
						{
							flags |= TARGET;
							flags |= HEADER;
							flags |= VALUE;
							flags |= DIGIT;
							flags |= HEX;
						} else if (c == '!' || c == '#' || c == '$' || c == '%' || c == '&' ||
                	c == '\'' || c == '*' || c == '+' || c == '-' || c == '.' ||
                 	c == '^' || c == '_' || c == '`' || c == '|' || c == '~')
						{
							flags |= TARGET;
							flags |= HEADER;
							flags |= VALUE;
						} else if (c == ':' || c == '/' || c == '?' || c == '#' ||
                	c == '[' || c == ']' || c == '@' ||
                 	c == '(' || c == ')' || c == ',' || c == ';' ||
                  	c == '=')
						{
        		    flags |= TARGET;
              		flags |= VALUE;
                	flags |= SEPARATOR;
        		} else if (c == ' ' || c == '\t')
          		{
            		flags |= VALUE;
              		flags |= SEPARATOR;
                	flags |= WHITESPACE;
            	} else if (c >= 33 && c <= 126)
             	{
              		flags |= VALUE;
              	} else if (c >= 0x80) {
               		flags |= VALUE;
               	}
					}
					return table;
				}();

		// Parsing
		std::stringstream	getVectorLine();
		std::vector<char>	getBodyData();
		void				extractReqLine();
		void				parseQuery();
		void				extractHeaders(bool body);

		// Body processing
		void	readBody(int serverSocket, int epollFd);
		void	extractChunkedBody();
		void	extractBody();
		void	extractBoundary();
		void	extractContentLength();
		void	extractMultipartFormData();

		// Checking functions
		bool	checkRequest(std::stringstream& line);
		void	checkHeaders(std::string_view key, std::string_view value);

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

		void	startParsing(std::vector<char>& request, int serverSocket, int epollFd);
};
