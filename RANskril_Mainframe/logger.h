#pragma once
#include "commons.h"

class Logger {
private:
	Logger();
	~Logger();
public:
	static Logger& GetInstance();
	Logger(const Logger&) = delete;
	Logger& operator=(const Logger&) = delete;

	void Write(std::wstring subcomponent, std::wstring message, int level);
};