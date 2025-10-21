#pragma once
#include <string>
#include <format>

/// <summary>
/// Log用
/// </summary>
namespace Logger {

	/// <summary>
	/// 文字列をログとして出力する
	/// </summary>
	void Log(const std::string& outputString);

	/// <summary>
	/// ワイド文字列をログとして出力する
	/// </summary>
	void Log(const std::wstring& outputString);

	/// <summary>
	/// ワイド文字列（UTF-16）を通常の文字列（UTF-8）へ変換する
	/// </summary>
	std::string ConvertString(const std::wstring& str);

	/// <summary>
	/// 通常の文字列（UTF-8）をワイド文字列（UTF-16）へ変換する
	/// </summary>
	std::wstring ConvertString(const std::string& str);
}
