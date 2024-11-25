#pragma once
#include <string>
#include <format>

namespace Logger {

	void Log(const std::string& outputString);
	void Log(const std::wstring& outputString);

	std::string ConvertString(const std::wstring& str);
	std::wstring ConvertString(const std::string& str);
}
