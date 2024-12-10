#pragma once

#include <string>

class CgiHandler
{
public:
	CgiHandler();
	~CgiHandler();

	std::string executeCgi(const std::string& path);
};