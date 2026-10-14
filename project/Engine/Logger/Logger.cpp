#include "Engine/Logger/Logger.h"

#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <mutex>

namespace {

	// シェーダの並列コンパイルから同時に呼ばれる
	void Write(const std::string& line) {
		static std::mutex mutex;
		static std::ofstream file = [] {
			const std::filesystem::path directory = "../generated/logs";
			std::error_code error;
			std::filesystem::create_directories(directory, error);
			return std::ofstream(directory / "FujiEngine.log", std::ios::trunc);
		}();

		std::lock_guard lock(mutex);
		OutputDebugStringA(line.c_str());
		if (file.is_open()) {
			// 落ちる直前の行まで残したいので毎回書き出す
			file << line << std::flush;
		}
	}
}

void Logger::Log(const std::string& outputString) {
	Write(outputString + "\n");
}

void Logger::Log(const std::wstring& outputString) {
	Write(ConvertString(outputString) + "\n");
}

void Logger::LogF(const char* format, ...) {
	char buffer[1024];

	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	Write(std::string(buffer) + "\n");
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

std::wstring Logger::ConvertString(const std::string& str) {
	if (str.empty()) {
		return std::wstring();
	}

	auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
	if (sizeNeeded == 0) {
		return std::wstring();
	}
	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
	return result;
}
