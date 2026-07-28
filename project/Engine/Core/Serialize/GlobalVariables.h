#pragma once
#include <variant>
#include <map>
#include <json_fwd.hpp>	// json は private な型エイリアスにしか使わないので前方宣言で足りる
#include "Engine/Math/Matrix/MatrixCalculation.h"

namespace Core {

	/// <summary>
	/// グローバル変数を管理するクラス
	/// </summary>
	class GlobalVariables {
	public:
		static GlobalVariables* GetInstance();

		/// <summary>
		/// グローバル変数1項目分の値、int / float / Vector3 のいずれかを持つ
		/// </summary>
		struct Item {
			std::variant<int32_t, float, Math::Vector3> value;
		};

		/// <summary>
		/// 項目をまとめるグループ、1グループが1つのJsonファイルに対応する
		/// </summary>
		struct Group {
			std::map<std::string, Item> items;
		};

		/// <summary>
		/// グループの作成
		/// </summary>
		/// <param name="groupName">グループ名</param>
		void CreateGroup(const std::string& groupName);

		// 値のセット(int)
		void SetValue(const std::string& groupName, const std::string& key, int32_t value);
		// 値のセット(float)
		void SetValue(const std::string& groupName, const std::string& key, float value);
		// 値のセット(Vector3)
		void SetValue(const std::string& groupName, const std::string& key, const Math::Vector3& value);

		/// <summary>
		/// ファイルに書き出し
		/// </summary>
		/// <param name="groupName">グループ</param>
		void SaveFile(const std::string& groupName);

		/// <summary>
		/// ディレクトリの全ファイル読み込み
		/// </summary>
		void LoadFiles();

		/// <summary>
		/// ファイルから読み込む
		/// </summary>
		/// <param name="groupName">グループ</param>
		void LoadFile(const std::string& groupName);

		// 項目の追加(int)
		void AddItem(const std::string& groupName, const std::string& key, int32_t value);
		// 項目の追加(float)
		void AddItem(const std::string& groupName, const std::string& key, float value);
		// 項目の追加(Vector3)
		void AddItem(const std::string& groupName, const std::string& key, Math::Vector3& value);

		// 値の取得
		int32_t GetIntValue(const std::string& groupName, const std::string& key) const;
		float GetFloatValue(const std::string& groupName, const std::string& key) const;
		Math::Vector3 GetVector3Value(const std::string& groupName, const std::string& key) const;

		/// <summary>
		/// 毎フレーム処理
		/// </summary>
		void Update();


		//グローバル変数の保存先ファイルパス
		const std::string kDirectoryPath = "resource/GlobalVariables/";


	private:
		GlobalVariables() = default;
		~GlobalVariables() = default;
		GlobalVariables(const GlobalVariables&) = delete;
		GlobalVariables& operator=(const GlobalVariables&) = delete;


		using json = nlohmann::json;


		// 全データ
		std::map<std::string, Group> datas_;
	};

}
