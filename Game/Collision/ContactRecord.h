#pragma once

#include<vector>
#include<cstdint>

/// <summary>
/// 接触記録
/// </summary>
class ContactRecord{
public:
	/// <summary>
	/// 履歴チェック
	/// </summary>
	/// <param name="number"></param>
	void AddRecord(uint32_t number);

	/// <summary>
	/// 履歴をチェック
	/// </summary>
	/// <param name="number"></param>
	/// <returns></returns>
	bool CheckRecord(uint32_t number);

	/// <summary>
	/// 履歴の抹消
	/// </summary>
	void Clear();

private:
	//履歴
	std::vector<uint32_t> record_;
};
