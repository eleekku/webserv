#include "HttpParser.hpp"
#include <cerrno>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <filesystem>
#include <sys/socket.h>

HttpParser::HttpParser() : _pos(0), _method_enum(UNKNOWN), _status(0), _contentLength(0) {}

// Getters
std::map<std::string, std::string>	HttpParser::getHeaders() { return _headers;}
std::string							HttpParser::getMethodString() { return _method;}
std::string							HttpParser::getTarget() { return _target;}
uint8_t								HttpParser::getMethod() { return _method_enum;}
int									HttpParser::getStatus() { return _status;}

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

void	HttpParser::extractReqLine()
{
	getVectorLine() >> _method >> _target >> _version;
	if (_version.back() == '\r')
		_version.pop_back();
	if (_method == "GET")
		_method_enum = GET;
	else if (_method == "POST")
		_method_enum = POST;
	else if (_method == "DELETE")
		_method_enum = DELETE;
	else
		_status = 405;
}

void checkHeaders(std::string key, std::string value)
{
	if (key.empty() || value.empty())
		throw std::invalid_argument("Empty header key or value.");
	if (key.back() != ':')
		throw std::invalid_argument("Header key does not end with a colon.");
	if (key.back() == ' ')
		throw std::invalid_argument("Header key ends with a space.");
}

void	HttpParser::extractHeaders()
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
		line >> key;
		while (line >> word)
			value += word + ' ';
		value.pop_back();
		checkHeaders(key, value);
		key.pop_back();
		if (value.back() == '\r')
			value.pop_back();
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
	}
}

void	HttpParser::extractBody()
{
	std::cout << "EXTRACTING BODY\n";
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

void	HttpParser::readBody(int serverSocket)
{
	int	bytesRead = 0;
	char buffer[BUFFER_SIZE] = {0};

	std::cout << "Server Socket: " << serverSocket << std::endl;

	_request.clear();
	_pos = 0;
	while (true)
	{
		bytesRead = recv(serverSocket, buffer, BUFFER_SIZE, 0);
		 if (bytesRead == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			perror("Error reading from client socket");
			throw std::runtime_error("Error reading from client socket");
		}
		_request.insert(_request.end(), buffer, buffer + bytesRead);
	}
}

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

std::map<std::string, std::string>	HttpParser::extractBodyHeaders()
{
	std::stringstream	line;
	std::string 		word;
	std::string 		key;
	std::string 		value;
	std::map<std::string, std::string> bodyHeaders;

	while (true)
	{
		line = getVectorLine();
		if (line.str() == "\r")
			break;
		line >> key;
		while (line >> word)
			value += word + ' ';
		value.pop_back();
		checkHeaders(key, value);
		key.pop_back();
		bodyHeaders.emplace(key, value);
		value.clear();
	}
	for (const auto& pair : bodyHeaders)
	{
		std::cout << pair.first << " : " << pair.second << std::endl;
	}
	return bodyHeaders;
}

std::string	extractFilename(std::map<std::string, std::string>& bodyHeaders)
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
	std::map<std::string, std::string>	bodyHeaders;
	std::string	filename;
	std::stringstream line;
	std::vector<char> content;
	std::string		lineStr;

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
			std::cout << "Extracting body headers\n";
			bodyHeaders = extractBodyHeaders();
			filename = extractFilename(bodyHeaders);
			while (true)
			{
				line = getVectorLine();
				lineStr = line.str();
				if (lineStr.back() == '\r')
					lineStr.pop_back();
				if (lineStr == _boundary || lineStr == _boundary + "--")
					break;
				content.insert(content.end(), lineStr.begin(), lineStr.end());
				content.push_back('\n');
			}
			content.pop_back();
			if (!filename.empty())
			{
				std::ofstream outFile("www/uploads/" + filename, std::ios::binary);
				if (!outFile)
					throw std::runtime_error("Failed to open file for writing: " + filename);
				outFile.write(content.data(), content.size());
				outFile.close();
			} else {
				_formFields[bodyHeaders["name"]] = content.data();
			}
		}
		if (lineStr == _boundary + "--")
			break;
	}
}

void	HttpParser::startParsing(std::vector<char>& request, int serverSocket)
{
	_request = request;
	extractReqLine();
	extractHeaders();
	std::cout << _method << " " << _target << " " << _version << std::endl;
	for (const auto& pair : _headers)
	{
		std::cout << pair.first << " : " << pair.second << std::endl;
	}
	if (_method == "POST")
	{
		extractContentLength();
		readBody(serverSocket);
		if (_request.size() > 0)
			extractBody();
	}
}
