#pragma once

#include <string>
#include <map>

// Network Constants
static const int    MAX_EVENTS = 10;
static const int    BUFFER_SIZE = 8192;
static const int    MAX_REQUEST_SIZE = 24576;
static const int    CONNECTION_TIMEOUT = 2000;    // mseconds
static const int    CGI_TIMEOUT = 5;           // seconds
static const int    MAX_CLIENTS = 1024;
static const int    BACKLOG = 10;              // Listen backlog

// Size Limits
static const size_t MAX_HEADER_SIZE = 8000;
static const size_t MAX_URI_LENGTH = 4096;
static const size_t MAX_METHOD_LENGTH = 6;
static const size_t MAX_CONNECTIONS = 1000;

// Default Values
static const char*  DEFAULT_ERROR_PAGE = "./www/error/error.html";
static const char*  DEFAULT_INDEX = "index.html";
static const int    DEFAULT_PORT = 8080;
static const char*  DEFAULT_HOST = "127.0.0.1";

// File Paths
static const char*  CGI_DIR = "./cgi-bin/";
static const char*  LOG_FILE = "./logs/server.log";

// HTTP Methods
enum HttpMethod {
    DELETE = 0,
    GET = 1,
    POST = 2,
    UNKNOWN = 3
};

// Parsing States
enum ParsingState {
	start = 1,
	readingRequest = 2,
	parsingRequest = 3,
	startBody = 4,
	readingBody = 5,
	parsingBody = 6,
	done = 7,
	checkingRequest = 8,
	error = 0
};

// HTTP Status Messages
static const std::map<int, std::string> HTTP_STATUS_MESSAGES = {
	{100, "Continue"},
    {102, "Processing"},
    {200, "OK"},
    {201, "Created"},
    {204, "No Content"},
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {408, "Request Timeout"},
    {413, "Payload Too Large"},
    {414, "URI Too Long"},
    {415, "Unsupported Media Type"},
    {431, "Request Header Fields Too Large"},
    {500, "Internal Server Error"},
    {501, "Not Implemented"},
    {502, "Bad Gateway"},
    {503, "Service Unavailable"},
    {504, "Gateway Timeout"},
    {505, "HTTP Version Not Supported"}
};

// MIME Types
static const std::map<std::string, std::string> MIME_TYPES = {
    {".html", "text/html"},
    {".htm", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif", "image/gif"},
    {".svg", "image/svg+xml"},
    {".txt", "text/plain"},
    {".pdf", "application/pdf"},
    {".zip", "application/zip"},
    {".ico", "image/x-icon"}
};
