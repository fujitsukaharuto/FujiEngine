#pragma once
#include <json_fwd.hpp>
#include "Engine/GameObject/GameObject.h"

namespace GameObject {

	/// <summary>
	/// シーンに入ったとき、プレイヤー等を置く位置と向き
	/// </summary>
	/// <remarks>配置データに名前付きで置く。ChangeScene で入口の名前を渡すと、遷移先でその位置が使われる</remarks>
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
