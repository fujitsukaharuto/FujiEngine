#pragma once
#ifdef _DEBUGMODE
#include <string>
#include "Engine/Math/Matrix/MatrixCalculation.h"

namespace Editor {

	/// <summary>
	/// Jsonのセーブ / ロードを行う編集UI
	/// </summary>
	/// <remarks>読み書き自体は Core::JsonSerializer が行う。こちらはポップアップとUndoコマンドの発行を担当する</remarks>
	class JsonEditorUI {
	public:

		//========================================================================*/
		//* Transform
		/// <summary>Transformのセーブ時ポップアップ</summary>
		/// <param name="transform">位置</param>
		static void ShowSaveTransformPopup(const Math::Trans& transform);
		/// <summary>Transformのロード時ポップアップ、読み込み時にUndoコマンドを発行する</summary>
		/// <param name="transform">位置</param>
		static void ShowLoadTransformPopup(Math::Trans& transform);

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
#endif // _DEBUGMODE
