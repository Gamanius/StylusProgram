#pragma once

#include <string>
#include <queue>
#include "render/RenderHandler.h"

#ifndef LOGGER_H
#define LOGGER_H

#undef ERROR

enum LOGGER_TYPE {
	INFO = 0,
	WARNING,
	ERROR
};


// this class is widley inefficent in every way imaginable so only use in debug mode...plz 
class Logger {
public:
	enum PRINT_TARGET {
		STD_OUT,
		DIRECT2D_CONTEXT,
		FILE_OUT
	};
	static std::queue<std::wstring> formatedMessage;

	static RenderHandler::Direct2DContext* inWindowDebugOutput;
	static bool enableInWindowDebugOutput;

	static bool forcePrint;
	static PRINT_TARGET printtarget;

	static std::wstring loggerTypeToString(LOGGER_TYPE t) {
		switch (t) {
		case INFO:
			return L"Info";
		case WARNING:
			return L"Warning";
		case ERROR:
			return L"Error";
		}
		return L"INVALID";
	}

public:
	Logger() = delete;
	Logger(const Logger& l) = delete;
	~Logger() = delete;
	Logger& operator=(const Logger&) = delete;

	static void init(PRINT_TARGET target);

	static void add(const std::wstring& s, LOGGER_TYPE t = LOGGER_TYPE::INFO);
	static void add(const int& s, LOGGER_TYPE t = LOGGER_TYPE::INFO);
	static void add(const double& s, LOGGER_TYPE t = LOGGER_TYPE::INFO);
	static void add(const unsigned int& s, LOGGER_TYPE t = LOGGER_TYPE::INFO);
	static void add(const unsigned long& s, LOGGER_TYPE t = LOGGER_TYPE::INFO);
	static void add(const long& s, LOGGER_TYPE t = LOGGER_TYPE::INFO);
	static void add(const float& s, LOGGER_TYPE t = LOGGER_TYPE::INFO);
	static void add(const bool& s, LOGGER_TYPE t = LOGGER_TYPE::INFO);
	static void add(const char& s, LOGGER_TYPE t = LOGGER_TYPE::INFO);
	static void add(const wchar_t& s, LOGGER_TYPE t = LOGGER_TYPE::INFO);
	static void add(const char* s, LOGGER_TYPE t = LOGGER_TYPE::INFO);
	static void add(const wchar_t* s, LOGGER_TYPE t = LOGGER_TYPE::INFO);
	static void add(const std::string& s, LOGGER_TYPE t = LOGGER_TYPE::INFO);

	static void err(const std::wstring& s);

	static void print(std::wostream& out);
	static void print(PRINT_TARGET target);
	static void print(RenderHandler::Direct2DContext* context);
	static void print();

	static void setDirect2DContext(RenderHandler::Direct2DContext* context);
};

#endif // !LOGGER_H
