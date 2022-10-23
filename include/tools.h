#pragma once
#include "../pch.h"

namespace Tools {

	//	hookenz && oleksii via Stack Overflow
	//	https://stackoverflow.com/questions/5840148/how-can-i-get-a-files-size-in-c
	long GetFileSize(std::string filename)
	{
		struct stat stat_buf;
		int rc = stat(filename.c_str(), &stat_buf);
		return rc == 0 ? stat_buf.st_size : -1;
	}

	std::string GetCurrentPath()
	{
		char buffer[MAX_PATH] = { 0 };
		GetModuleFileNameA(NULL, buffer, MAX_PATH);
		std::string::size_type pos = std::string(buffer).find_last_of("\\/");
		return std::string(buffer).substr(0, pos);
	}

	std::string ParseError(DWORD ErrorCode)
	{
		if (!ErrorCode)
			return std::string();

		LPSTR messageBuffer = nullptr;

		size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, ErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

		std::string message(messageBuffer, size);

		LocalFree(messageBuffer);

		return message;
	}
}