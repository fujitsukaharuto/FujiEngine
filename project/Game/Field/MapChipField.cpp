#include"Field/MapChipField.h"

#include<map>
#include<fstream>
#include<sstream>
#include <cassert>

namespace {
	std::map<std::string, MapChipType> mapChipTable = {
	{"0",MapChipType::Normal_Block},
	};
}

void MapChipField::ResetMapChipData(){
	mapChipData_.data.clear();
	mapChipData_.data.resize(kNumBlockVirtical);
	for (std::vector<MapChipType>& mapChipDataLine:mapChipData_.data){
		mapChipDataLine.resize(kNumBlockHorizontal);
	}
}

void MapChipField::LoadMapChipCsv(const std::string& filiPath){
	//マップチップデータをリセット
	ResetMapChipData();

	//ファイルを開く
	std::ifstream file;
	file.open(filiPath);
	assert(file.is_open());

	//マップチップcsv
	std::stringstream mapChipCsv;
	//ファイルの内容を文字列ストリームにコピー
	mapChipCsv << file.rdbuf();
	//ファイルを閉じる
	file.close();

	//csvからマップチップデータを読み込む
	for (uint32_t i = 0; i < kNumBlockVirtical; i++){
		std::string line;
		std::getline(mapChipCsv,line);

		//1行分の文字列ストリームに変換して解析しやすくする
		std::istringstream line_stream(line);
		for (uint32_t j = 0; j < kNumBlockHorizontal; j++){

			std::string word;
			std::getline(line_stream, word, ',');

			if (mapChipTable.contains(word)){
				mapChipData_.data[i][j] = mapChipTable[word];
			}

		}
	}
}

MapChipType MapChipField::GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex){
	return mapChipData_.data[yIndex][xIndex];
}
