#include "HttpParser.hpp"
#include "Constants.hpp"
#include "HandleRequest.hpp"
#include <algorithm>
#include <cerrno>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <filesystem>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

HttpParser::HttpParser() : _state(start), _pos(0), _totalBytesRead(0),
	_method_enum(UNKNOWN), _status(200), _maxBodySize(0), _contentLength(0), _lastSeen(time(nullptr)), _keepAlive(false), _chunked(false)
{
	_request.reserve(BUFFER_SIZE);
	_filename.clear();
	_chunkSize = 0;
}

// Getters
map_t		HttpParser::getHeaders() { return _headers;}
std::string	HttpParser::getMethodString() { return _method;}
std::string	HttpParser::getTarget() { return _target;}
uint8_t		HttpParser::getMethod() { return _method_enum;}
int			HttpParser::getStatus() { return _status;}
std::string	HttpParser::getQuery() { return _query;}
std::string HttpParser::getBody() { return _body;}
size_t		HttpParser::getContentLength() { return _contentLength;}
bool		HttpParser::getKeepAlive() { return _keepAlive;}

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

std::vector<char>	HttpParser::getChunkLine()
{
	std::vector<char>	line;

	line.reserve(_chunkSize);
	size_t	i = 0;
	while (i++ < _chunkSize)
	{
		line.push_back(_request[_pos]);
		_pos++;
	}
	_pos += 2;
	return line;
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
		_status = 501;
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
		}
		else
			_headers.emplace(key, value);
		value.clear();
	}
}

void	HttpParser::extractContentLength()
{
	if (_chunked)
		return ;
	if (_headers.contains("Content-Length"))
	{
		std::stringstream ss(_headers["Content-Length"]);
		ss >> _contentLength;
	} else {
		_status = 411;
		_state = error;
		return;
	}
	if (_contentLength == 0)
	{
		_state = error;
		_status = 400;
	}
}

void	HttpParser::extractStringBody()
{
	std::vector<char>	content;
	std::vector<char>	lineVec;

	content.reserve(_contentLength);
	while (true)
	{
		lineVec.clear();
		lineVec = getBodyData();
		content.insert(content.end(), lineVec.begin(), lineVec.end());
		if (content.size() >= _contentLength)
		{
			break;
		}
	}
	_body.assign(content.begin(), content.end());
	_status = 200;
}

void	HttpParser::extractOctetStream()
{
	std::vector<char>	content;
	std::vector<char>	lineVec;
	std::string			fileExtension;

	fileExtension.clear();
	if (_validMimeType)
	{
		std::string	contentType = _headers["Content-Type"];
  		for (const auto& mime : MIME_TYPES)
    	{
     		if (contentType.find(mime.second) != std::string::npos)
		    {
				fileExtension = mime.first;
				break ;
		    }
		}
	}
	content.reserve(_contentLength);
	while (true)
	{
		lineVec.clear();
		lineVec = getBodyData();
		content.insert(content.end(), lineVec.begin(), lineVec.end());
		if (content.size() >= _contentLength)
		{
			break;
		}
	}
	std::ofstream outFile(_uploadFolder + "/upload" + fileExtension, std::ios::binary);
	if (!outFile)
		throw std::runtime_error("Failed to open file for writing: upload");
	outFile.write(content.data(), content.size());
	outFile.close();
	_status = 201;
}

void	HttpParser::extractBody()
{
	std::string contentType = _headers["Content-Type"];

	if (contentType.find("application/x-www-form-urlencoded") != std::string::npos)
		extractStringBody();
	else if (contentType.find("multipart/form-data") != std::string::npos)
		extractMultipartFormData();
	else if (contentType.find("application/octet-stream") != std::string::npos)
		extractOctetStream();
	else {
		_validMimeType = false;
	    for (const auto& mime : MIME_TYPES)
	    {
	        if (contentType.find(mime.second) != std::string::npos)
	        {
	            _validMimeType = true;
	            extractOctetStream();
	            break;
	        }
	    }
        if (!_validMimeType)
        {
            _status = 415;
            _state = error;
            throw std::runtime_error("Unsupported Media Type");
        }
	}
	_state = done;
}

void	HttpParser::extractChunkedBody()
{
	std::vector<char>	content;
	std::vector<char>	lineVec;
	std::stringstream	chunkSizeStream;

	content.reserve(_contentLength);
	while (true)
	{
		chunkSizeStream = getVectorLine();
		size_t chunkSize;
		chunkSizeStream >> std::hex >> chunkSize;
		if (chunkSize == 0)
			break;
		size_t bytesRead = 0;
		while (bytesRead < chunkSize)
		{
			lineVec.clear();
			lineVec = getBodyData();
			content.insert(content.end(), lineVec.begin(), lineVec.end());
			bytesRead += lineVec.size();
		}
	}
	_request.clear();
	_request = std::move(content);
	_pos = 0;
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

std::string	HttpParser::extractFilename()
{
	std::string	filename;
	std::string	disposition;
	size_t		filenamePos;
	size_t		start;
	size_t		end;

	if (_bodyHeaders.contains("Content-Disposition"))
	{
		disposition = _bodyHeaders["Content-Disposition"];
		filenamePos = disposition.find("filename=\"");
		if (filenamePos != std::string::npos)
		{
			start = filenamePos + 10;
			end = disposition.find("\"", start);
			filename = disposition.substr(start, end - start);
			if (filename == "")
			{
				throw std::runtime_error("Empty filename in multipart/form-data");
				_state = done;
				_status = 400;
			}
		}
	}
	return filename;
}

void	HttpParser::extractMultipartFormData()
{
	std::string							filename;
	std::stringstream					line;
	std::vector<char>					content(_contentLength);
	std::vector<char>					lineVec;
	size_t								vecSize;
	std::string							lineStr;

	if (!std::filesystem::exists(_uploadFolder))
		std::filesystem::create_directory(_uploadFolder);
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
			extractHeaders(true);
			filename = extractFilename();
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
				std::ofstream outFile(_uploadFolder + filename, std::ios::binary);
				if (!outFile)
				{
					_status = 500;
					_state = error;
					throw std::runtime_error("Failed to open file for writing: " + filename);
				}
				outFile.write(content.data(), content.size());
				outFile.close();
				_status = 201;
			} else {
				_status = 400;
				_state = error;
				throw std::runtime_error("No filename in multipart/form-data");
			}
		if (lineStr == _boundary + "--")
			break;
		}
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
	_state = parsingRequest;
}

void HttpParser::readBody(int clientfd)
{
	int bytesRead = 0;
	char buffer[BUFFER_SIZE] = {0};

	bytesRead = recv(clientfd, buffer, BUFFER_SIZE, 0);
	if (bytesRead > 0)
	{
		_totalBytesRead += bytesRead;
		_request.insert(_request.end(), buffer, buffer + bytesRead);
	}
	if (bytesRead == 0)
		_state = parsingBody;
	if (bytesRead == -1)
	{
		_state = error;
	//	perror("Error reading from client socket");
	//	throw std::runtime_error("Error reading from client socket");
	}
	if (_totalBytesRead == _contentLength)
		_state = parsingBody;
	if (_totalBytesRead > _contentLength)
	{
		_status = 400;
		_state = error;
		throw std::runtime_error("Content-Length mismatch");
	}
	if (_totalBytesRead > _maxBodySize)
	{
		_status = 413;
		_state = error;
		throw std::runtime_error("Request entity too large");
	}
}

std::string	HttpParser::getUploadPath(ConfigFile& conf, int serverIndex)
{
	std::string location = _target.substr(0, _target.find('/', 1));
	LocationConfig locConfig = findKey(location, serverIndex, conf);
	std::string root = locConfig.root;
	root.erase(0, 1);
	root.push_back('/');
	return root;
}

void	HttpParser::checkLimitMethods(ConfigFile& conf, int serverIndex)
{
	std::string location = _target.substr(0, _target.find('/', 1));
	LocationConfig locConfig = findKey(location, serverIndex, conf);
	std::string_view limit = locConfig.limit_except;
	if (limit.find(_method) == limit.npos)
	{
		_status = 405;
		throw std::runtime_error("Method not allowed");
	}
}

void	HttpParser::readRequest(int clientfd)
{
	int			bytesRead = 0;
	char		buffer[BUFFER_SIZE] = {0};
	std::string	str("\r\n\r\n");

	bytesRead = recv(clientfd, buffer, BUFFER_SIZE, 0);
	if (bytesRead == 0)
	{
		_state = checkingRequest;
		return ;
	}
	else if (bytesRead == -1)
	{
		_state = error;
		throw std::runtime_error("Error reading from client socket");
	}
	else
	{
		_totalBytesRead += bytesRead;
		_request.insert(_request.end(), buffer, buffer + bytesRead);
		if (_request.size() >= 4)
		{
			// Possible optimization starting from previous end - 4
			auto it = std::search(_request.begin(), _request.end(), str.begin(), str.end());
			if (it != _request.end())
			{
				std::copy(it + 4, _request.end(), std::back_inserter(_tmp));
				_state = checkingRequest;
				return ;
			}
		}
		if (_request.size() > MAX_REQUEST_SIZE)
		{
			const char *needle = "\r\n\r\n";
			_request.insert(_request.end(), needle, needle + 4);
			_state = checkingRequest;
		}
	}
}

bool isValidMethodChar(char c) {
    return (c >= 'A' && c <= 'Z');
}

bool isValidTargetChar(char c) {
    return (c >= 'A' && c <= 'Z') ||
           (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') ||
           c == '-' || c == '.' || c == '_' || c == '~' ||
           c == '/' || c == '?' || c == '=' || c == '&';
}

bool isValidHeaderKeyChar(char c) {
    return (c >= 33 && c <= 126) && c != ':';
}

bool isValidHeaderValueChar(char c) {
    return (c >= 32 && c <= 126) || c == '\t';
}

bool	HttpParser::checkValidCharacters()
{
	for (char c : _method)
	{
		if (!isValidMethodChar(c))
			return false;
	}
	for (char c : _target)
	{
		if (!isValidTargetChar(c))
			return false;
	}
	for (const auto& pair : _headers)
	{
		for (char c : pair.first)
		{
			if (!isValidHeaderKeyChar(c))
				return false;
		}
		for (char c : pair.second)
		{
			if (!isValidHeaderValueChar(c))
				return false;
		}
	}
	return true;
}

void	HttpParser::isKeepAlive()
{
	if (_headers.contains("Connection"))
	{
		if (_headers["Connection"] == "keep-alive")
			_keepAlive = true;
	}
}

bool	HttpParser::checkTimeout()
{
	time_t now = time(nullptr);
	if (difftime(now, _lastSeen) > CONNECTION_TIMEOUT)
	{
		_state = error;
		_status = 408;
		return true;
	}
	return false;
}

void	HttpParser::startChunkedBody()
{
	_totalBytesRead = _tmp.size();
	_request = std::move(_tmp);
	if (_headers["Content-Type"].find("multipart/form-data") != std::string::npos)
	{
		_filename = extractFilename();
		if (_filename.empty())
		{
			_status = 400;
			_state = error;
			throw std::runtime_error("No filename in multipart/form-data");
		}
	} else {
		_filename = "upload";
	}
	std::ofstream outFile(_uploadFolder + _filename, std::ios::binary);
	outFile.close();
	parseChunkedBody();
}

void	HttpParser::parseChunkedBody()
{
	std::ofstream 		outFile;
	std::stringstream	chunkSizeStream;
	std::vector<char>	content;

	outFile.open(_uploadFolder + _filename, std::ios::app);
	if (!outFile)
	{
		_status = 500;
		_state = error;
		throw std::runtime_error("Failed to open file for writing: " + _filename);
	}
	while (true)
	{
		content.clear();
		if (_chunkSize == 0)
		{
			chunkSizeStream = getVectorLine();
			chunkSizeStream >> std::hex >> _chunkSize;
		}
		if ((_request.size() - _pos) < _chunkSize + 2)
		{
			_state = readingChunkedBody;
			outFile.close();
			return ;
		}
		if (_chunkSize == 0)
		{
			_state = done;
			outFile.close();
			return ;
		}
		content = getChunkLine();
		outFile.write(content.data(), content.size());
		_chunkSize = 0;
	}
	outFile.close();
}

void	HttpParser::readChunkedBody(int clientfd)
{
	int bytesRead = 0;
	char buffer[BUFFER_SIZE] = {0};

	bytesRead = recv(clientfd, buffer, BUFFER_SIZE, 0);
	if (bytesRead > 0)
	{
		_totalBytesRead += bytesRead;
		_request.insert(_request.end(), buffer, buffer + bytesRead);
	}
	if (bytesRead == 0)
	{
		_state = parsingChunkedBody;
		return ;
	}
	if (bytesRead == -1)
	{
		_state = error;
		throw std::runtime_error("Error reading from client socket");
	}
	if (_totalBytesRead > _maxBodySize)
	{
		_status = 413;
		_state = error;
		throw std::runtime_error("Request entity too large");
	}
	if ((_request.size() - _pos) < _chunkSize + 2)
	{
		_state = readingChunkedBody;
		return ;
	}
	_state = parsingChunkedBody;
}

void	HttpParser::startBodyFunction(ConfigFile& conf, int serverIndex)
{
	_request.clear();
	_pos = 0;
	_uploadFolder = getUploadPath(conf, serverIndex);
	if (!std::filesystem::exists(_uploadFolder))
	{
		_status = 404;
		_state = error;
		throw std::runtime_error("Folder does not exist");
	}
	if (_headers.contains("Transfer-Encoding") &&_headers["Transfer-Encoding"] == "chunked")
	{
		_chunked = true;
		startChunkedBody();
		return ;
	}
	extractContentLength();
	if (_contentLength == 0)
		return;
	_request.reserve(_contentLength);
	_request = std::move(_tmp);
	_totalBytesRead = _request.size();
	if (_request.size() >= _contentLength)
		_state = parsingBody;
	else
		_state = readingBody;
}

bool	HttpParser::startParsing(int clientfd, ConfigFile& conf, int serverIndex)
{
	try {
		_lastSeen = time(nullptr);
		while (true)
		{
			switch (_state)
			{
				case start:
					_maxBodySize = conf.getMax_body(serverIndex);
					_state = readingRequest;
					readRequest(clientfd);
					break;
				case readingRequest:
					readRequest(clientfd);
					break;
				case checkingRequest:
					checkReadRequest();
					break;
				case parsingRequest:
					extractReqLine();
					checkLimitMethods(conf, serverIndex);
					parseQuery();
					extractHeaders(false);
					if (checkValidCharacters() == false)
					{
						_status = 400;
						_state = error;
						break;
					}
					isKeepAlive();
					if (_method_enum == POST)
						_state = startBody;
					else
						_state = done;
					break;
				case startBody:
					startBodyFunction(conf, serverIndex);
					if (_state == done)
						_status = 201;
					break;
				case readingChunkedBody:
					readChunkedBody(clientfd);
					break;
				case parsingChunkedBody:
					parseChunkedBody();
					if (_state == done)
						_status = 201;
					break;
				case readingBody:
					readBody(clientfd);
					break;
				case parsingBody:
					if (_request.size() > 0)
						extractBody();
					break;
				case done:
					_request.clear();
					break;
				case error:
					break;
			}
			if (_state == done || _state == error || _state == readingRequest
				|| _state == readingBody || _state == readingChunkedBody)
				break;
		}
	} catch (std::exception &e) {
		_state = error;
		std::cerr << e.what() << std::endl;
	}
	try {
		if (_state == done || _state == error)
		{
			return true;
		}
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
	}
	return false;
}
