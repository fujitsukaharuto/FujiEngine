#pragma once
#include <variant>
#include <map>
#include <json.hpp>
#include "Math/Matrix/MatrixCalculation.h"


class JsonSerializer {
public:
	using json = nlohmann::json;

	static void ShowSaveTransformPopup(const Trans& transform);
	static void ShowLoadTransformPopup(Trans& transform);
	static void SerializeTransform(const Trans& transform, const std::string& filePath);
	static bool DeserializeTransform(const std::string& filePath, Trans& outTransform, bool isCreateCommand = false);



};
