#include "JsonSerializer.h"
#include <json.hpp>
#include <fstream>
#include <filesystem>

using namespace Core;
using namespace Math;


namespace {
	constexpr int JSON_INDENT_WIDTH = 4;
}


void JsonSerializer::SerializeTransform(const Trans& transform, const std::string& filePath) {
	json json;
	// Vector3をそれぞれ配列として保存
	json["translate"] = { transform.translate.x, transform.translate.y, transform.translate.z };
	json["rotate"] = { transform.rotate.x, transform.rotate.y, transform.rotate.z };
	json["scale"] = { transform.scale.x, transform.scale.y, transform.scale.z };

	std::filesystem::path dir = "resource/Json";
	std::filesystem::create_directories(dir); // ディレクトリが無ければ作成
	std::filesystem::path fullPath = dir / (filePath);

	// 書き込み
	std::ofstream ofs(fullPath);
	if (ofs.is_open()) {
		ofs << json.dump(JSON_INDENT_WIDTH); // インデント付きで出力
		ofs.close();
	}
}

bool JsonSerializer::DeserializeTransform(const std::string& filePath, Trans& outTransform) {
	std::filesystem::path dir = "resource/Json";
	std::filesystem::path fullPath = dir / (filePath);
	std::ifstream ifs(fullPath);
	if (!ifs.is_open()) {
		return false;
	}

	json json;
	ifs >> json;
	ifs.close();

	// JSON配列からVector3を復元
	if (json.contains("translate") && json["translate"].is_array()) {
		outTransform.translate.x = json["translate"][0];
		outTransform.translate.y = json["translate"][1];
		outTransform.translate.z = json["translate"][2];
	}
	if (json.contains("rotate") && json["rotate"].is_array()) {
		outTransform.rotate.x = json["rotate"][0];
		outTransform.rotate.y = json["rotate"][1];
		outTransform.rotate.z = json["rotate"][2];
	}
	if (json.contains("scale") && json["scale"].is_array()) {
		outTransform.scale.x = json["scale"][0];
		outTransform.scale.y = json["scale"][1];
		outTransform.scale.z = json["scale"][2];
	}
	return true;
}

void JsonSerializer::SerializeJsonData(const json& data, const std::string& filePath) {
	std::string fullPath = filePath;
	std::filesystem::path path(fullPath);
	if (path.extension() != ".json") {

		path.replace_extension(".json");
		fullPath = path.string();
	}
	std::ofstream file(fullPath);
	if (file.is_open()) {
		file << data.dump(JSON_INDENT_WIDTH);
		file.close();
	}
}

json JsonSerializer::DeserializeJsonData(const std::string& filePath) {
	std::ifstream file(filePath);
	if (!file.is_open()) {
		return json();
	}
	json data;
	file >> data;
	file.close();
	return data;
}
