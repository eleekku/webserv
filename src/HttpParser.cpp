#include "HttpParser.hpp"
#include <cerrno>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <filesystem>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

HttpParser::HttpParser() : _state(start), _pos(0), _totalBytesRead(0), _method_enum(UNKNOWN), _status(200), _contentLength(0) {}

// Getters
map_t		HttpParser::getHeaders() { return _headers;}
std::string	HttpParser::getMethodString() { return _method;}
std::string	HttpParser::getTarget() { return _target;}
uint8_t		HttpParser::getMethod() { return _method_enum;}
int			HttpParser::getStatus() { return _status;}
std::string	HttpParser::getQuery() { return _query;}
std::string HttpParser::getBody() { return _body;}

std::stringstream HttpParser::getVectorLine()
{
	std::stringstream line;

	while (_pos < _request.size() && _request[_pos] != '\n')
	{
		line << _request[_pos];
		++_pos;
	}
	++_pos;
	return line;
}

std::vector<char>	HttpParser::getBodyData()
{
	std::vector<char>	body;

	while (_pos < _request.size())
	{
		body.push_back(_request[_pos]);
		++_pos;
		if (body.back() == '\n')
			break;
	}
	return body;
}

void	HttpParser::extractReqLine()
{
	std::stringstream	line = getVectorLine();
	std::string			remaining;

	if (!(line >> _method >> _target >> _version))
	{
		_status = 400;
		return ;
	}
	if (checkRequest(line) == false)
		return ;
	if (_method == "GET")
		_method_enum = GET;
	else if (_method == "POST")
		_method_enum = POST;
	else if (_method == "DELETE")
		_method_enum = DELETE;
	else {
		_status = 400;
		throw std::invalid_argument("Invalid method: " + _method);
	}
}

bool	HttpParser::checkRequest(std::stringstream& line)
{
	std::string	remaining;

	if (line >> remaining)
	{
		_status = 400;
		throw std::invalid_argument("Bad request.");
	}
    if (_target.empty() || _method.empty() || _version.empty())
    {
		_status = 400;
		throw std::invalid_argument("Bad request.");
    }
    if (_version.back() == '\r')
		_version.pop_back();
	if (_version != "HTTP/1.1")
	{
    	_status = 505;
     	throw std::invalid_argument("Bad request.");
	}
	if (_target.front() != '/')
	{
		_status = 400;
		throw std::invalid_argument("Bad request.");
	}
	if (_method.size() > 6) // 6 is the length of the longest method
	{
		_status = 501;
		throw std::invalid_argument("Method too long: " + _method);
	}
	if (_target.size() > 4096)
	{
		_status = 414;
		throw std::invalid_argument("Target too long: " + _target);
	}
	return true;
}

void HttpParser::checkHeaders(std::string_view key, std::string_view value)
{
	if (key.empty() || value.empty())
	{
		_status = 400;
		throw std::invalid_argument("Empty header key or value.");
	}
	if (key.back() != ':')
	{
		_status = 400;
		throw std::invalid_argument("Header key does not end with a colon.");
	}
	if (key.back() == ' ')
	{
		_status = 400;
		throw std::invalid_argument("Header key ends with a space.");
	}
	if (value.front() == ':')
	{
		_status = 400;
		throw std::invalid_argument("Header key end with a space.");
	}
	if (key.size() > 8000 || value.size() > 8000)
	{
		_status = 431;
		throw std::invalid_argument("Header key or value too long.");
	}
}

void	HttpParser::extractHeaders(bool body)
{
	std::stringstream	line;
	std::string			word;
	std::string 		key;
	std::string 		value;

	while (true)
	{
		line = getVectorLine();
		if (line.str() == "\r")
			break;
		if (line.str().empty())
		 	throw std::runtime_error("Empty line in headers");
		if (_pos >= _request.size())
			throw std::runtime_error("Unexpected end of request");
		line >> key;
		while (line >> word)
			value += word + ' ';
		value.pop_back();
		checkHeaders(key, value);
		key.pop_back();
		if (value.back() == '\r')
			value.pop_back();
		if (body)
		{
			_bodyHeaders.emplace(key, value);
			std::cout << key << ": " << _bodyHeaders[key] << std::endl;
		}
		else
			_headers.emplace(key, value);
		value.clear();
	}
}

void	HttpParser::extractContentLength()
{
	if (_headers.contains("Content-Length"))
	{
		std::stringstream ss(_headers["Content-Length"]);
		ss >> _contentLength;
	} else {
		_contentLength = 0;
		_status = 411;
		throw std::runtime_error("Content-Length header not found");
	}
}

void	HttpParser::extractBody()
{
	if (_headers.contains("Content-Type"))
	{
		if (_headers["Content-Type"].find("multipart/form-data") != std::string::npos)
			 extractMultipartFormData();
	}
	else if (_headers.contains("Transfer-Encoding") &&
		_headers["Transfer-Encoding"] == "chunked")
	{
		std::cout << "WE ARE CHUNKEDDDDDDDDDDD\n";
		// extractChunkedBody(request);
	}
	else if (_contentLength > 0)
	{
		std::vector<char> content(_contentLength);
		// _ssrequest.read(content.data(), _contentLength);
		// if (static_cast<size_t>(_ssrequest.gcount()) != _contentLength)
		// {
		// 	throw std::runtime_error("Failed to read complete body");
		// }
	}
}

// void	HttpParser::extractChunkedBody(std::vector<char>& request)
// {
// 	std::string	line;
// 	int chunkSize;

// 	while (true)
// 	{
// 		std::getline(request, line);
// 		if (line.back() == '\r')
// 			line.pop_back();
// 		std::vector<char> ss;
// 		ss << std::hex << line;
// 		ss >> chunkSize;
// 		if (chunkSize == 0)
// 			break;
// 		std::string chunk;
// 		chunk.resize(chunkSize);
// 		request.read(&chunk[0], chunkSize);
// 		_body += chunk;
// 		std::getline(request, line);
// 	}
// 	std::getline(request, line);
// }

void	HttpParser::extractBoundary()
{
	if (_headers.contains("Content-Type"))
	{
		std::string	contentType = _headers["Content-Type"];
		size_t	pos = contentType.find("boundary=");
		if (pos != std::string::npos)
		{
			_boundary = "--" + contentType.substr(pos + 9);
			if (_boundary.back() == '\r')
				_boundary.pop_back();
		}
	}
}

std::string	extractFilename(map_t& bodyHeaders)
{
	std::string	filename;
	std::string	disposition;
	size_t		filenamePos;
	size_t		start;
	size_t		end;

	if (bodyHeaders.contains("Content-Disposition"))
	{
		disposition = bodyHeaders["Content-Disposition"];
		filenamePos = disposition.find("filename=\"");
		if (filenamePos != std::string::npos)
		{
			start = filenamePos + 10;
			end = disposition.find("\"", start);
			filename = disposition.substr(start, end - start);
		}
	}
	return filename;
}

void	HttpParser::extractMultipartFormData()
{
	std::string							filename;
	std::stringstream					line;
	std::vector<char>					content;
	std::vector<char>					lineVec;
	size_t								vecSize;
	std::string							lineStr;

	content.reserve(_contentLength);
	if (!std::filesystem::exists("www/uploads"))
		std::filesystem::create_directory("www/uploads");
	extractBoundary();
	if (_boundary.empty())
		throw std::runtime_error("No boundary found in multipart/form-data");
	while (true)
	{
		line = getVectorLine();
		lineStr = line.str();
		if (lineStr.back() == '\r')
			lineStr.pop_back();
		if ( lineStr == _boundary)
		{
			content.clear();
			std::cout << "Extracting body headers\n";
			extractHeaders(true);
			filename = extractFilename(_bodyHeaders);
			while (true)
			{
				lineVec.clear();
				lineVec = getBodyData();
				vecSize = lineVec.size();
				if (vecSize == _boundary.size() + 2 || vecSize == _boundary.size() + 4)
				{
					std::string vecAsString(lineVec.begin(), lineVec.end() - 2);
					if (vecAsString == _boundary || vecAsString == _boundary + "--")
					{
						lineStr = vecAsString;
						break;
					}
				}
				content.insert(content.end(), lineVec.begin(), lineVec.end());
			}
			content.pop_back();
			content.pop_back();
			if (!filename.empty())
			{
				std::ofstream outFile("www/uploads/" + filename, std::ios::binary);
				if (!outFile)
					throw std::runtime_error("Failed to open file for writing: " + filename);
				outFile.write(content.data(), content.size());
				outFile.close();
			} else {
				_body.assign(content.begin(), content.end());
			}
		}
		if (lineStr == _boundary + "--")
			break;
	}
}

void	HttpParser::parseQuery()
{
	size_t	pos = _target.find('?');
	if (pos != std::string::npos)
	{
		_query = _target.substr(pos + 1);
		_target = _target.substr(0, pos);
	}
	else
		_query = "";
}

void	HttpParser::checkReadRequest()
{
	if (_request.size() > MAX_REQUEST_SIZE)
	{
		const char *needle = "\r\n\r\n";
		_request.insert(_request.end(), needle, needle + 4);
		return ;
	}
	if (_request.size() >= 4)
	{
		std::string end(_request.end() - 4, _request.end());
		if (end != "\r\n\r\n")
		{
			_status = 400;
			_state = error;
			return ;
		}
	}
	_state = parsingRequest;
}

void HttpParser::readRequest(int clientfd, bool body)
{
	int bytesRead = 0;
	char buffer[BUFFER_SIZE] = {0};

	std::cout << "Reading request..." << std::endl;
	bytesRead = recv(clientfd, buffer, BUFFER_SIZE, 0);
	if (bytesRead == 0)
	{
		if (body)
			if (_totalBytesRead == _contentLength)
				_state = parsingBody;
			else
				throw std::runtime_error("Unexpected end of request");
		else
			_state = checkingRequest;
		return ;
	}
	else if (bytesRead == -1)
	{
		_state = error;
		perror("Error reading from client socket");
		throw std::runtime_error("Error reading from client socket");
	}
	else
	{
		_totalBytesRead += bytesRead;
		std::cout << _totalBytesRead << " / " << _contentLength << std::endl;
		_request.insert(_request.end(), buffer, buffer + bytesRead);
		// Check if the request is too big or has other issues
	}
}

bool	HttpParser::startParsing(int clientfd)
{
	std::cout << _state << std::endl;
	try {
		switch (_state)
		{
			case start:
				_request.reserve(BUFFER_SIZE);
				readRequest(clientfd, false);
				break;
			case readingRequest:
				readRequest(clientfd, false);
				break;
			case checkingRequest:
				checkReadRequest();
				break;
			case parsingRequest:
				extractReqLine();
				parseQuery();
				extractHeaders(false);
				if (_method == "POST")
					_state = startBody;
				else
					_state = done;
				break;
			case startBody:
				extractContentLength();
				_request.clear();
				_request.reserve(_contentLength);
				_pos = 0;
				_totalBytesRead = 0;
				readRequest(clientfd, true);
				break;
			case readingBody:
				readRequest(clientfd, true);
				break;
			case parsingBody:
				if (_request.size() > 0)
				{
					extractBody();
					_status = 201;
				}
				break;
			case done:
				break;
			case error:
				break;
		}
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
	}
	if (_state == done || _state == error)
		return true;
	return false;
	//std::cout << _method << " " << _target << " " << _version << std::endl;
	//for (const auto& pair : _headers)
	//{
	//	std::cout << pair.first << " : " << pair.second << std::endl;
	//}
}
