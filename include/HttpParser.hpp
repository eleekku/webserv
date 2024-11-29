#pragma once
#include <cstddef>
#include <string_view>
#include <unordered_map>
#include <string>

#define MAX_REQ_LINE_SIZE 8000
#define MAX_METHOD_SIZE 6

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

		void	checkReqLineErrors(); // to do
		size_t	parseMethod();
		size_t	parseTarget(size_t index);
		size_t	parseHttp(size_t index);
		size_t	parseHeaders(size_t index);
		void	parseBody(size_t index);
		void	parseChunkedBody(size_t index);
	public:
		HttpParser(size_t size);
		void 					parseRequest(std::string_view request);
		e_http_method			getMethod();
		std::string_view		getMethodString();
		std::string_view		getTarget();
		bool 					isValidHttp();
		std::unordered_map<std::string_view, std::string_view> getHeaders();
		std::string				getBody();

};
