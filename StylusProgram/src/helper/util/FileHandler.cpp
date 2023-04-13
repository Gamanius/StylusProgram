#include "FileHandler.h"
#include "Logger.h"

FileHandler::File::File(const File& f) {
	data = new byte[f.size];
	memcpy((void*)data, (void*)f.data, f.size);
}

FileHandler::File& FileHandler::File::operator=(const File& p) {
	data = new byte[p.size];
	memcpy((void*)data, (void*)p.data, p.size);
	return *this;
}

FileHandler::File::File(File&& p) {
	data = p.data;
	size = p.size;

	p.data = nullptr;
	p.size = 0;
}

FileHandler::File& FileHandler::File::operator=(File&& p) {
	data = p.data;
	size = p.size;

	p.data = nullptr;
	p.size = 0;
	return *this;
}

FileHandler::File::~File() {
	delete[] data;
}


FileHandler::File FileHandler::openFile(const std::wstring& s) {
	// open file using CreateFileW from the win32 api
	HANDLE hFile = CreateFileW(s.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) { 
		Logger::add(L"Failed to open file: " + s, LOGGER_TYPE::ERROR); 
		return File(); 
	}

	DWORD fileSize = GetFileSize(hFile, NULL);
	if (fileSize == INVALID_FILE_SIZE) {
		Logger::add(L"Failed to get file size: " + s, LOGGER_TYPE::ERROR);
		CloseHandle(hFile);
		return File();
	}

	File file;
	file.data = new byte[fileSize];
	file.size = fileSize;

	if (!ReadFile(hFile, file.data, fileSize, NULL, NULL)) {
		Logger::add(L"Failed to read file: " + s, LOGGER_TYPE::ERROR);
		CloseHandle(hFile);
		file.~File();
		return File();
	}
	return std::move(file);
}
