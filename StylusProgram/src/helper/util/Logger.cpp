#include "Logger.h"
#include <Windows.h>
#include <iostream>
#include <locale>
#include <codecvt>

std::queue<std::wstring> Logger::formatedMessage;
RenderHandler::Direct2DContext* Logger::inWindowDebugOutput;
bool Logger::enableInWindowDebugOutput;
bool Logger::forcePrint;
Logger::PRINT_TARGET Logger::printtarget;

void Logger::init(PRINT_TARGET target) {
	inWindowDebugOutput = nullptr;
	enableInWindowDebugOutput = false;
#ifdef _DEBUG
	forcePrint = true;
#else 
	forcePrint = false;
#endif // _DEBUG
	printtarget = target;
}

void Logger::add(const std::wstring& s, LOGGER_TYPE t) {
	std::wstring msg;
	msg.reserve(s.size() + 10);
	msg += L"[" + loggerTypeToString(t) + L"]: ";
	msg.append(s);

	formatedMessage.push(msg);

	if (forcePrint) {
		print();
	}
}

void Logger::add(const int& s, LOGGER_TYPE t) {
	add(std::to_wstring(s), t);
}

void Logger::add(const double& s, LOGGER_TYPE t) {
	add(std::to_wstring(s), t);
}

void Logger::add(const unsigned int& s, LOGGER_TYPE t) {
	add(std::to_wstring(s));
}

void Logger::add(const unsigned long& s, LOGGER_TYPE t) {
	add(std::to_wstring(s));
}

void Logger::add(const long& s, LOGGER_TYPE t) {
	add(std::to_wstring(s));
}

void Logger::add(const float& s, LOGGER_TYPE t) {
	add(std::to_wstring(s));
}

void Logger::add(const bool& s, LOGGER_TYPE t) {
	add(std::to_wstring(s));
}

void Logger::add(const char& s, LOGGER_TYPE t) {
	add(std::to_wstring(s));
}

void Logger::add(const wchar_t& s, LOGGER_TYPE t) {
	add(std::to_wstring(s));
}

void Logger::add(const char* s, LOGGER_TYPE t) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wide = converter.from_bytes(s);
	add(wide);
}

void Logger::add(const wchar_t* s, LOGGER_TYPE t) {
	add(std::wstring(s));
}

void Logger::add(const std::string& s, LOGGER_TYPE t) {
	add(s.c_str());
}

void Logger::err(const std::wstring& s) {
	add(s, LOGGER_TYPE::ERROR);
}


void Logger::print(std::wostream& out) {
	while (!formatedMessage.empty()) {
		out << formatedMessage.front() << L"\n";
		formatedMessage.pop();
	}
}

void Logger::print() {
	switch (printtarget) {
	case PRINT_TARGET::DIRECT2D_CONTEXT:
	{
		if (inWindowDebugOutput != nullptr)
			print(inWindowDebugOutput);
		break;
	}
	case PRINT_TARGET::STD_OUT:
	{
		print(std::wcout);
		break;
	}
	}
}

void Logger::print(PRINT_TARGET target) {
	switch (target) {
	case PRINT_TARGET::DIRECT2D_CONTEXT:
	{
		if (inWindowDebugOutput != nullptr)
			print();
		break;
	}
	case PRINT_TARGET::STD_OUT:
	{
		print(std::wcout);
		break;
	}
	}
}

void Logger::print(RenderHandler::Direct2DContext* context) {
	while (!formatedMessage.empty()) {
		context->addDebugText(formatedMessage.front());
		formatedMessage.pop();
	}
}

void Logger::setDirect2DContext(RenderHandler::Direct2DContext* context) {
	inWindowDebugOutput = context;
}
