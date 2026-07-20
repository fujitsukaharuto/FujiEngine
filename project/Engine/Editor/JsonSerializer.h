#pragma once
#include <variant>
#include <map>
#include <json.hpp>
#include "Engine/Math/Matrix/MatrixCalculation.h"

namespace Editor {
	struct EditorObj;
}
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
		/// <param name="transform">位置</param>
		static void ShowSaveTransformPopup(const Math::Trans& transform);
		/// <summary>Transformのロード時ポップアップ</summary>
		/// <param name="transform">位置</param>
		static void ShowLoadTransformPopup(Math::Trans& transform);
		/// <summary>Transformのデータ出力</summary>
		/// <param name="transform">位置</param> <param name="filePath">ファイルパス</param>
		static void SerializeTransform(const Math::Trans& transform, const std::string& filePath);
		/// <summary>Transformのデータ読み込み</summary>
		/// <param name="filePath">ファイルパス</param> <param name="outTransform">位置</param> <param name="isCreateCommand">作るかどうか</param>
		/// <returns>bool</returns>
		static bool DeserializeTransform(const std::string& filePath, Math::Trans& outTransform, bool isCreateCommand = false);

		//========================================================================*/
		//* EditorObj
		/// <summary>EditorObjのセーブ時ポップアップ</summary>
		/// <param name="obj">オブジェクト</param>
		static void ShowSaveEditorObjPopup(const Editor::EditorObj& obj);
		/// <summary>EditorObjのロード時ポップアップ</summary>
		/// <param name="obj">オブジェクト</param>
		/// <returns>bool</returns>
		static bool ShowLoadEditorObjPopup(Editor::EditorObj& obj);
		/// <summary>EditorObjのデータ出力</summary>
		/// <param name="obj">オブジェクト</param> <param name="filePath">ファイルパス</param>
		static void SerializeEditorObj(const Editor::EditorObj& obj, const std::string& filePath);
		/// <summary>EditorObjのデータ読み込み</summary>
		/// <param name="filePath">ファイルパス</param> <param name="obj">オブジェクト</param> <param name="isCreateCommand">作るかどうか</param>
		/// <returns>bool</returns>
		static bool DeserializeEditorObj(const std::string& filePath, Editor::EditorObj& obj, bool isCreateCommand = false);

		//========================================================================*/
		//* JsonData
		
		/// <summary>Jsonのデータの出力</summary>
		/// <param name="data">データ</param> <param name="filePath">ファイルパス</param>
		static void SerializeJsonData(const json& data, const std::string& filePath);
		/// <summary>Jsonのデータの読み込み</summary>
		/// <param name="filePath">ファイルパス</param>
		/// <returns>json</returns>
		static json DeserializeJsonData(const std::string& filePath);

		//========================================================================*/
		//* Popup
		/// <summary>セーブ時のポップアップ</summary>
		/// <param name="success">成功したか</param>
		static void SavedPopup(bool& success);
		/// <summary>ロード時のポップアップ</summary>
		/// <param name="success">成功したか</param>
		static void LoadedPopup(bool& success);
		/// <summary>ロードエラー時のポップアップ</summary>
		/// <param name="error">エラーかどうか</param> <param name="filePath">ファイルパス</param>
		static void LoadErrorPopup(bool& error, const std::string& filePath);

	};
}