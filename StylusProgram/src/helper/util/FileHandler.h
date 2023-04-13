#pragma once

#include <string>
#include "Util.h"

#ifndef FILE_HANDLER_H
#define FILE_HANLDER_H

namespace FileHandler {
	struct File {
		byte* data = nullptr;
		unsigned long size = 0;

		File() = default;
		File(const File& f);
		File& operator=(const File& p);
		File(File&& p);
		File& operator=(File&& p);
		~File();
	};

	File openFile(const std::wstring& s);
}

#endif // !FILE_HANDLER_H
