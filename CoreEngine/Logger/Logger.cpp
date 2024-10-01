#include "Logger.h"

#include <Windows.h>


void Logger::Log(const std::string& outputString) {
	OutputDebugStringA((outputString+"/n").c_str());
}

void Logger::Log(const std::wstring& outputString) {
	OutputDebugStringA((ConvertString(outputString) + "/n").c_str());
}

std::string Logger::ConvertString(const std::wstring& str) {
	if (str.empty()) {
		return std::string();
	}

	auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
	if (sizeNeeded == 0) {
		return std::string();
	}
	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
	return result;
}
