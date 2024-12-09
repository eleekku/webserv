#include "HttpParser.hpp"
#include <stdexcept>
#include <iostream>
#include <sstream>

// Constructor
HttpParser::HttpParser(size_t size) : _max_body_size(size) {}

//Getters
e_http_method HttpParser::getMethod() { return _method;}
std::string_view HttpParser::getMethodString() {return _method_string;}
std::string_view HttpParser::getTarget() { return _target;}
bool HttpParser::isValidHttp() { return _valid_http;}
std::unordered_map<std::string_view, std::string_view> HttpParser::getHeaders() { return _headers;}
std::string HttpParser::getBody() { return _body;}

void	HttpParser::checkReqLineErrors()
{
	if (_request.size() > MAX_REQ_LINE_SIZE)
		throw std::invalid_argument("Request line too long.");
}

size_t HttpParser::parseMethod()
{
	size_t index = _request.find(' ');
	_method_string = _request.substr(0, index);
	if (_method_string == "GET")
		_method = GET;
	else if (_method_string == "POST")
		_method = POST;
	else if (_method_string == "DELETE")
		_method = DELETE;
	else
		_method = UNKNOWN;
	return index;
}

size_t HttpParser::parseTarget(size_t index)
{
	while (index < _request.size() && std::isspace(_request[index]))
		index++;
	size_t new_index = _request.find(' ', index);
	_target = _request.substr(index, new_index - (index));
	return new_index;
}

size_t HttpParser::parseHttp(size_t index)
{
	while (index < _request.size() && std::isspace(_request[index]))
		index++;
	size_t new_index = _request.find("\r\n", index);
	std::string_view sv = _request.substr(index, new_index - (index));
	if (sv == "HTTP/1.1")
		_valid_http = true;
	else
		_valid_http = false;
	return new_index;
}

size_t HttpParser::parseHeaders(size_t index)
{
	size_t iter, end_line;
	index += 2;
	size_t end_headers = _request.find("\r\n\r\n", index);
	while (index < end_headers)
	{
		iter = index;
		end_line = _request.find("\r\n", index);
		while (iter < end_line && _request[iter] != ':')
			iter++;
		if (_request[iter - 1] == ' ')
		{
			_status = 400;
			throw std::invalid_argument("Wrong headers.");
		}
		std::string_view key = _request.substr(index, iter - index);
		iter++;
		while (iter < end_line && _request[iter] == ' ')
			iter++;
		index = iter;
		std::string_view value = _request.substr(index, end_line - index);
		if (_request[end_line - 1] == ' ')
		{
			iter = 1;
			while (_request[end_line - iter - 1] == ' ')
				iter++;
			value.remove_suffix(iter);
		}
		_headers.emplace(key, value);
		index = end_line + 2;
	}
	return end_headers + 4;
}

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
	else if (_headers.contains("Content-Length"))
	{
		std::string content_length(_headers["Content-Length"]);
		_body_size = std::stoi(content_length);
		if (_body_size > _max_body_size)
		{
			_status = 400;
			throw std::invalid_argument("Max body size exceeded.");
		}
		_body = _request.substr(index, _body_size);
	}
}

void HttpParser::parseRequest(std::string_view request)
{
	_request = request;
	size_t index;
	try {
		index = parseMethod();
		if (_method == UNKNOWN)
			throw std::invalid_argument("Wrong method.");
		index = parseTarget(index);
		index = parseHttp(index);
		if (_valid_http == false)
			throw std::invalid_argument("Wrong HTTP version.");
		index = parseHeaders(index);
		parseBody(index);
	} catch (std::invalid_argument &e) {
		std::cerr << e.what() << std::endl;
	}
}
