#include "../include/HttpParser.hpp"
#include <cstddef>
#include <exception>
#include <stdexcept>
#include <iostream>
#include <sstream>

// Constructor
HttpParser::HttpParser(size_t size) : _max_body_size(size), _status(200) {}

//Getters
e_http_method HttpParser::getMethod() { return _method;}
//std::string_view HttpParser::getMethodString() {return _method_string;}
//std::string_view HttpParser::getTarget() { return _target;}
unsigned int	HttpParser::getStatus() { return _status;}
bool HttpParser::isValidHttp() { return _valid_http;}
std::unordered_map<std::string, std::string> HttpParser::getHeaders() { return _headers;}
std::string HttpParser::getBody() { return _body;}
size_t	HttpParser::getBodySize() { return _body_size;}
/*
void	HttpParser::checkReqParse()
{
	if (_request.size() > MAX_REQ_LINE_SIZE)
	{
		_status = 400;
		throw std::invalid_argument("Bad request. Request line too long.");
	}
}
*/
void HttpParser::parseMethod()
{
	if (_method_str == "GET")
		_method = GET;
	else if (_method_str == "POST")
		_method = POST;
	else if (_method_str == "DELETE")
		_method = DELETE;
	else
		_method = UNKNOWN;
}

/*
size_t HttpParser::parseTarget(size_t index)
{
	while (index < _request.size() && std::isspace(_request[index]))
		index++;
	size_t new_index = _request.find(' ', index);
	if (new_index == std::string_view::npos)
	{
		_status = 400;
		throw std::invalid_argument("Bad Request. Missing space after target.");
	}
	_target = _request.substr(index, new_index - (index));
	return new_index;
}

size_t HttpParser::parseHttp(size_t index)
{
	while (index < _request.size() && std::isspace(_request[index]))
		index++;
	size_t new_index = _request.find("\r\n", index);
	if (new_index == std::string_view::npos)
	{
		_status = 400;
		throw std::invalid_argument("Bad Request. Missing CRLF after HTTP version.");
	}
	std::string_view sv = _request.substr(index, new_index - (index));
	if (sv == "HTTP/1.1")
		_valid_http = true;
	else
		_valid_http = false;
	return new_index;
}
*/
void HttpParser::parseHeaders(std::stringstream& request)
{
	std::string line;
	std::getline(request, line);
	std::string key;
	std::string value;
	size_t sep;

	while (std::getline(request, line) && line != "\r")
	{
		std::cout << line << std::endl;
		sep = line.find(":");
		if (sep == std::string::npos || line.back() != '\r')
			throw std::invalid_argument("Wrong header format.");
		key = line.substr(0, sep);
		value = line.substr(sep + 2);
		line.pop_back();
		_headers.emplace(key, value);
	}
	for (const auto& [key, value] : getHeaders()) {
        std::cout << "<" << key << ">:<" << value << ">\n";
    }
}

/*
void	HttpParser::parseChunkedBody(size_t index)
{
	size_t		pos = index;

	try {
		while (true)
		{
			size_t chunk_size_end = _request.find("\r\n", pos);
			if (chunk_size_end == std::string::npos)
				throw std::invalid_argument("Incomplete chunk size line");
			std::string_view chunk_size_sv = _request.substr(pos, chunk_size_end - pos);
			size_t semicolon_pos = chunk_size_sv.find(";");
			if (semicolon_pos != std::string::npos)
				chunk_size_sv = chunk_size_sv.substr(0, semicolon_pos);

			size_t chunk_size;

			std::string chunk_size_str(chunk_size_sv);
			std::istringstream iss(chunk_size_str);
			if (!(iss >> std::hex >> chunk_size))
				throw std::invalid_argument("Invalid chunk size format");

			if (chunk_size == 0)
			{
				pos = chunk_size_end + 2;
				break;
			}

			pos = chunk_size_end + 2;
			if (pos + chunk_size + 2 > _request.length())
				throw std::invalid_argument("Incomplete chunk data");

			_body.append(_request.substr(pos, chunk_size));
			pos += chunk_size + 2;
		}
	} catch (const std::exception& e) {
		_status = 400;
		throw;
	}
}

void	HttpParser::parseBody(size_t index)
{
	if (_headers.contains("Transfer-Encoding"))
		parseChunkedBody(index);
}
*/
void HttpParser::checkMethod()
{
	unsigned int i = 0;

	while (i < _method_str.size())
	{
		if (!isToken(_method_str[i]))
		{
			_status = 400;
			throw std::invalid_argument("Bad Request.");
		}
		i++;
	}
	if (_method == UNKNOWN)
	{
		_status = 501;
		throw std::invalid_argument("Not Immplemented.");
	}
}
/*
void HttpParser::checkKey(std::string_view sv)
{
	for (unsigned int i = 0; i < sv.size(); i++)
	{
		if (!isHeader(sv[i]))
		{
			_status = 400;
			throw std::invalid_argument("Bad request.");
		}
	}
}

void HttpParser::checkValue(std::string_view sv)
{
	for (unsigned int i = 0; i < sv.size(); i++)
	{
		if (!isValue(sv[i]))
		{
			_status = 400;
			throw std::invalid_argument("Bad request.");
		}
	}
}

void HttpParser::checkHeaders()
{
	if (_headers.count("Host") != 1)
	{
		_status = 400;
		throw std::invalid_argument("Bad Request.");
	}
	if (_headers.contains("Content-Length"))
	{
		std::string content_length(_headers["Content-Length"]);
		_body_size = std::stoi(content_length);
		if (_body_size > _max_body_size)
		{
			_status = 413	;
			throw std::invalid_argument("Payload Too Large");
		}
	}
	if (_headers["Content-Type"].find("boundary") != std::string::npos)
	{
		size_t pos = _headers["Content-Type"].find("=");
		_boundary = "--" + std::string(_headers["Content-Type"].substr(pos));
		std::cout << "Boundary is:" << _boundary << "\n";
	}
}

void	HttpParser::checkBody()
{

}

void HttpParser::checkTarget()
{
	unsigned int i = 0;

	if (_target.size() > MAX_URI_SIZE)
	{
		_status = 414;
		throw std::invalid_argument("URI Too Long.");
	}
	while(i < _target.size())
	{
		if (!isUri(_target[i]))
		{
			_status = 400;
			throw std::invalid_argument("Bad Request.");
		}
		i++;
	}
}
*/
void HttpParser::extractReqLine(std::stringstream& request)
{
	request >> _method_str >> _target_str >> _http_str;
	std::cout << "<" << _method_str << ">\n<" << _target_str << ">\n<" << _http_str << ">\n";
}

void HttpParser::parseRequest(std::stringstream& request)
{
	extractReqLine(request);
	parseMethod();
	checkMethod();
	parseHeaders(request);
	//size_t index;
	try {
		//index = parseTarget(index);
		//checkTarget();
		//index = parseHttp(index);
		//if (_valid_http == false)
		//{
		//	_status = 505;
		//	throw std::invalid_argument("HTTP Version Not Supported.");
		//}
		//index = parseHeaders(index);
		//checkHeaders();
	} catch (std::invalid_argument &e) {
		std::cerr << e.what() << std::endl;
	}
}

bool HttpParser::isToken(unsigned char c)
{
	return _TABLE[c] & TOKEN;
}

bool HttpParser::isWhitespace(unsigned char c)
{
	return _TABLE[c] & WHITESPACE;
}

bool HttpParser::isUri(unsigned char c)
{
	return _TABLE[c] & URI;
}

bool HttpParser::isHeader(unsigned char c)
{
	return _TABLE[c] & HEADER;
}

bool HttpParser::isValue(unsigned char c)
{
	return _TABLE[c] & VALUE;
}

bool HttpParser::isHex(unsigned char c)
{
	return _TABLE[c] & HEX;
}

bool HttpParser::isDigit(unsigned char c)
{
	return _TABLE[c] & DIGIT;
}
