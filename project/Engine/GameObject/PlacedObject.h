#pragma once
#include <optional>
#include <string>
#include <json_fwd.hpp>
#include "Engine/GameObject/GameObject.h"

namespace GameObject {

	/// <summary>
	/// 配置データから作られる、動きを持たないオブジェクト
	/// </summary>
	/// <remarks>地形や背景の置物用。見た目の設定もデータに持つので、コードを書かずに置ける。生成・保存は Scene::Level が行う</remarks>
	class PlacedObject : public GameObject {
	public:

		/// <summary>配置データ1件から生成する</summary>
		void Create(const nlohmann::json& data);
		/// <summary>モデル名だけで生成する。エディタからの追加用</summary>
		void Create(const std::string& modelName);
		/// <summary>配置データ1件分として書き出す</summary>
		nlohmann::json ToJson() const;

		void Update() override;
		/// <summary>名前と見た目の設定。ここで変えた値が保存される</summary>
		void ParameterGUI() override;

	private:

		/// <summary>モデルを作り、見た目の設定を当てる</summary>
		void Build();
		/// <summary>データに書いてある見た目の設定だけをモデルへ当てる</summary>
		void ApplyLook();

	private:

		/// <summary>配置データが持つ見た目の設定</summary>
		/// <remarks>optional は「書いてある時だけモデルの既定値を上書きする」ため。書いていない項目はモデル読み込み時の値のまま</remarks>
		struct Look {
			std::string model;
			/// <summary>AnimationModel で作るか。映り込み(isMirror)とアニメーションはこちらでしか使えない</summary>
			bool isAnime = false;
			/// <summary>モデルファイルのアニメーションを再生する(isAnime のときだけ)</summary>
			bool isAnimation = false;
			/// <summary>空欄ならモデルのテクスチャのまま</summary>
			std::string texture;
			std::optional<Math::Vector4> color;
			std::optional<Math::Vector2> uvScale;
			std::optional<float> roughness;
			std::optional<float> metallic;
			std::optional<float> environment;
			bool isMirror = false;
			std::optional<Graphics::LightMode> light;
		};
		Look look_;
	};

}
