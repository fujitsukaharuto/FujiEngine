#pragma once


#include <stdint.h>
#include <vector>
#include<string>

#include "Math/Vector/Vector3.h"


enum class MapChipType{
	Normal_Block,	//通常のブロック
};

/// <summary>
/// マップチップを縦横に並べたフィールドのデータ
/// </summary>
struct MapChipData{
	std::vector<std::vector<MapChipType>> data;
};

/// <summary>
/// マップチップフィールド
/// </summary>
class MapChipField{
public:
	MapChipField()= default;
	~MapChipField() = default;

	/// <summary>
	/// データのリセット
	/// </summary>
	void ResetMapChipData();

	/// <summary>
	/// csvファイル読み込み
	/// </summary>
	/// <param name="filePath"></param>
	void LoadMapChipCsv(const std::string& filePath);

	/// <summary>
	/// マップチップ種別の取得
	/// </summary>
	/// <param name="xIndex"></param>
	/// <param name="yIndex"></param>
	/// <returns></returns>
	MapChipType GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex);

public:
	//1ブロックのサイズ
	static inline const float kBlockWidth = 1.0f;
	static inline const float kBlockHeight = 1.0f;
	//ブロックの個数
	static inline const uint32_t kNumBlockVertical = 2;
	static inline const uint32_t kNumBlockHorizontal = 50;

public:
	MapChipData mapChipData_;
};
