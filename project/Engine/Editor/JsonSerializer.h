#pragma once
#include <variant>
#include <map>
#include <json.hpp>
#include "Math/Matrix/MatrixCalculation.h"

struct EditorObj;
using json = nlohmann::json;

class JsonSerializer {
public:

	// Transform
	static void ShowSaveTransformPopup(const Trans& transform);
	static void ShowLoadTransformPopup(Trans& transform);
	static void SerializeTransform(const Trans& transform, const std::string& filePath);
	static bool DeserializeTransform(const std::string& filePath, Trans& outTransform, bool isCreateCommand = false);

	// EditorObj
	static void ShowSaveEditorObjPopup(const EditorObj& obj);
	static bool ShowLoadEditorObjPopup(EditorObj& obj);
	static void SerializeEditorObj(const EditorObj& obj, const std::string& filePath);
	static bool DeserializeEditorObj(const std::string& filePath, EditorObj& obj, bool isCreateCommand = false);

	// JsonData
	static void SerializeJsonData(const json& data, const std::string& filePath);
	static json DeserializeJsonData(const std::string& filePath);

	// Popup
	static void SavedPopup(bool& success);
	static void LoadedPopup(bool& success);
	static void LoadErrorPopup(bool& error, const std::string& filePath);

};
