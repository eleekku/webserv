#pragma once
#include <cstddef>
#include <string_view>
#include <unordered_map>
#include <string>
#include <array>

#define MAX_REQ_LINE_SIZE 64000
#define MAX_METHOD_SIZE 6
#define MAX_URI_SIZE 8000

enum e_http_method {
	DELETE = 0,
	GET = 1,
	POST = 2,
	UNKNOWN = 3
};

class HttpParser {
	private:
		std::string_view								_request;
		e_http_method									_method;
		std::string_view								_method_string;
		std::string_view								_target;
		bool											_valid_http;
		std::unordered_map<std::string_view, std::string_view>	_headers;
		const size_t									_max_body_size;
		size_t											_body_size;
		std::string										_body;
		unsigned int									_status;

		static constexpr uint8_t TOKEN = 1 << 0;
		static constexpr uint8_t URI = 1 << 1;
		static constexpr uint8_t HEADER = 1 << 2;
		static constexpr uint8_t VALUE = 1 << 3;
		static constexpr uint8_t HEX = 1 << 4;
		static constexpr uint8_t DIGIT = 1 << 5;
		static constexpr uint8_t SEPARATOR = 1 << 6;
		static constexpr uint8_t WHITESPACE = 1 << 7;

		void	checkReqLineErrors(); // to do
		size_t	parseMethod();
		size_t	parseTarget(size_t index);
		size_t	parseHttp(size_t index);
		size_t	parseHeaders(size_t index);
		void	parseBody(size_t index);
		void	parseChunkedBody(size_t index);
		void	checkMethod();
		void	checkHeaders();
		void	checkTarget();
		void	checkKey(std::string_view sv);
		void	checkValue(std::string_view sv);

		static constexpr std::array<uint8_t, 256> _TABLE = []()
		{
			std::array<uint8_t, 256> table = {};

			for (int i = 0; i < 256; i++)
			{
				unsigned char c = static_cast<unsigned char>(i);
				uint8_t& flags = table[i];
				if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
				{
					flags |= TOKEN;
					flags |= URI;
					flags |= HEADER;
					flags |= VALUE;
					if ((c <= 'F') || (c >= 'a' && c <= 'f'))
						flags |= HEX;
				} else if ((c >= '0' && c <= '9'))
				{
					flags |= TOKEN;
					flags |= URI;
					flags |= HEADER;
					flags |= VALUE;
					flags |= DIGIT;
					flags |= HEX;
				} else if (c == '!' || c == '#' || c == '$' || c == '%' || c == '&' ||
                	c == '\'' || c == '*' || c == '+' || c == '-' || c == '.' ||
                 	c == '^' || c == '_' || c == '`' || c == '|' || c == '~')
				{
					flags |= TOKEN;
					flags |= URI;
					flags |= HEADER;
					flags |= VALUE;
				} else if (c == ':' || c == '/' || c == '?' || c == '#' ||
                	c == '[' || c == ']' || c == '@' ||
                 	c == '(' || c == ')' || c == ',' || c == ';' ||
                  	c == '=')
				{
        		    flags |= URI;
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

	public:
		HttpParser(size_t size);
		void 					parseRequest(std::string_view request);
		e_http_method			getMethod();
		std::string_view		getMethodString();
		std::string_view		getTarget();
		bool 					isValidHttp();
		std::unordered_map<std::string_view, std::string_view> getHeaders();
		std::string				getBody();

		// Character types checks
		static bool isWhitespace(unsigned char c);
		static bool isToken(unsigned char c);
		static bool isUri(unsigned char c);
		static bool isHeader(unsigned char c);
		static bool isValue(unsigned char c);
		static bool isHex(unsigned char c);
		static bool isDigit(unsigned char c);

};
