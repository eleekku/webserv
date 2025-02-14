# Webserv

A lightweight HTTP server implementation in C++. This project implements a web server capable of handling HTTP/1.1 requests, processing CGI scripts, and serving static content.

## Features

- HTTP/1.1 protocol support
- Multiple server configuration
- Non-blocking I/O with epoll
- CGI script execution
- Static file serving
- Configurable through configuration files
- Support for GET, POST, and DELETE methods
- Directory listing
- Custom error pages
- Keep-alive connections
- Request body size limits

## Requirements

- C++20 compiler
- Linux operating system (epoll support)
- Make

## Building

To build the project, run:

```bash
make
```

## Configuration

The server is configured through a configuration file. Here's an example configuration:

```nginx
server {
    listen 127.0.0.1:8080;
    server_name example.com;
    client_max_body_size 10M;
    error_page /www/error.html;

    location / {
        limit_except POST GET DELETE;
        root /www;
        autoindex on;
        index index.html;
    }

    location /cgi {
        limit_except POST GET DELETE;
        root /cgi-bin;
    }
}
```

### Configuration Options

- `listen`: IP address and port to listen on
- `server_name`: Server name identification
- `client_max_body_size`: Maximum allowed size for request bodies
- `error_page`: Custom error page path
- `location`: Path-specific configurations
  - `limit_except`: Allowed HTTP methods
  - `root`: Root directory for the location
  - `autoindex`: Enable/disable directory listing
  - `index`: Default index file

## Running

To run the server:

```bash
./webserv [config_file_path]
```

Example:
```bash
./webserv default.conf
```

## Features in Detail

### HTTP Methods
- GET: Retrieve resources
- POST: Submit data
- DELETE: Remove resources

### CGI Support
- Executes CGI scripts
- Handles both GET and POST requests to CGI
- Timeout management for CGI execution

### Error Handling
- Custom error pages
- Standard HTTP error codes
- Proper error reporting

### Security
- Request size limiting
- Method restrictions per location
- Basic input validation

## Implementation Details

- Non-blocking I/O using epoll
- Event-driven architecture
- MIME type support
- Keep-alive connection handling
- Timeout management
- Memory management with proper cleanup
