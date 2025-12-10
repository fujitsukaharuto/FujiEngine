#pragma once
#include <variant>
#include <map>
#include <json.hpp>
#include "Math/Matrix/MatrixCalculation.h"

struct EditorObj;
using json = nlohmann::json;

namespace Core {
	/// <summary>
	/// Json出力、読み込みのクラス
	/// </summary>
	class JsonSerializer {
	public:

		//========================================================================*/
		//* Transform
		/// <summary>Transformのセーブ時ポップアップ</summary>
		static void ShowSaveTransformPopup(const Math::Trans& transform);
		/// <summary>Transformのロード時ポップアップ</summary>
		static void ShowLoadTransformPopup(Math::Trans& transform);
		/// <summary>Transformのデータ出力</summary>
		static void SerializeTransform(const Math::Trans& transform, const std::string& filePath);
		/// <summary>Transformのデータ読み込み</summary>
		static bool DeserializeTransform(const std::string& filePath, Math::Trans& outTransform, bool isCreateCommand = false);

		//========================================================================*/
		//* EditorObj
		/// <summary>EditorObjのセーブ時ポップアップ</summary>
		static void ShowSaveEditorObjPopup(const EditorObj& obj);
		/// <summary>EditorObjのロード時ポップアップ</summary>
		static bool ShowLoadEditorObjPopup(EditorObj& obj);
		/// <summary>EditorObjのデータ出力</summary>
		static void SerializeEditorObj(const EditorObj& obj, const std::string& filePath);
		/// <summary>EditorObjのデータ読み込み</summary>
		static bool DeserializeEditorObj(const std::string& filePath, EditorObj& obj, bool isCreateCommand = false);

		//========================================================================*/
		//* JsonData
		/// <summary>Jsonのデータの出力</summary>
		static void SerializeJsonData(const json& data, const std::string& filePath);
		/// <summary>Jsonのデータの読み込み</summary>
		static json DeserializeJsonData(const std::string& filePath);

		//========================================================================*/
		//* Popup
		/// <summary>セーブ時のポップアップ</summary>
		static void SavedPopup(bool& success);
		/// <summary>ロード時のポップアップ</summary>
		static void LoadedPopup(bool& success);
		/// <summary>ロードエラー時のポップアップ</summary>
		static void LoadErrorPopup(bool& error, const std::string& filePath);

	};
}