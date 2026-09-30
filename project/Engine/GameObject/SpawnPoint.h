#pragma once
#include <json_fwd.hpp>
#include "Engine/GameObject/GameObject.h"

namespace GameObject {

	/// <summary>
	/// シーンに入ったとき、プレイヤー等を置く位置と向き
	/// </summary>
	/// <remarks>配置データ(Scene::Level)に名前付きで置く。ChangeScene で入口の名前を渡すと、遷移先ではその名前の出現位置が使われる(BaseScene::ApplySpawnPoint)。
	/// 見た目は持たず、_DEBUGMODE でだけ位置と向きを線で描く</remarks>
	class SpawnPoint : public GameObject {
	public:

		/// <summary>配置データ1件から生成する</summary>
		void Create(const nlohmann::json& data);
		/// <summary>配置データ1件分として書き出す</summary>
		nlohmann::json ToJson() const;

		void Draw(bool is = false) override;
		/// <summary>名前(=入口の名前)の編集</summary>
		void ParameterGUI() override;
	};

}
