#pragma once
#include <string>
// json は宣言(引数・戻り値)にしか使わないので前方宣言で足りる。実体が要る側は <json.hpp> を直接includeすること
#include <json_fwd.hpp>
#include "Engine/Math/Matrix/MatrixCalculation.h"

using json = nlohmann::json;

namespace Core {
	/// <summary>
	/// Jsonの読み書きを行うクラス
	/// </summary>
	/// <remarks>ランタイム機能なのでImGuiやEditorには依存しない。編集UIは Editor::JsonEditorUI にある</remarks>
	class JsonSerializer {
	public:

		//========================================================================*/
		//* Transform
		/// <summary>Transformのデータ出力</summary>
		/// <param name="transform">位置</param> <param name="filePath">ファイルパス</param>
		static void SerializeTransform(const Math::Trans& transform, const std::string& filePath);
		/// <summary>Transformのデータ読み込み</summary>
		/// <param name="filePath">ファイルパス</param> <param name="outTransform">位置</param>
		/// <returns>bool</returns>
		static bool DeserializeTransform(const std::string& filePath, Math::Trans& outTransform);

		//========================================================================*/
		//* JsonData
		/// <summary>Jsonのデータの出力</summary>
		/// <param name="data">データ</param> <param name="filePath">ファイルパス</param>
		static void SerializeJsonData(const json& data, const std::string& filePath);
		/// <summary>Jsonのデータの読み込み</summary>
		/// <param name="filePath">ファイルパス</param>
		/// <returns>json</returns>
		static json DeserializeJsonData(const std::string& filePath);

	};
}
