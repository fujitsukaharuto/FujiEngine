#pragma once
#include <json_fwd.hpp>
#include "Engine/GameObject/GameObject.h"

namespace GameObject {

	/// <summary>
	/// 間接光を焼く範囲の箱。位置が箱の中心、拡縮が大きさ(m)
	/// </summary>
	/// <remarks>回転は使わない</remarks>
	class GIVolume : public GameObject {
	public:

		/// <summary>配置データ1件から生成する。空なら既定の大きさ</summary>
		void Create(const nlohmann::json& data);
		/// <summary>配置データ1件分として書き出す</summary>
		nlohmann::json ToJson() const;

		void Draw(bool is = false) override;
		void ParameterGUI() override;

		Math::Vector3 GetCenter() const { return transform_.translate; }
		Math::Vector3 GetSize() const;
		float GetSpacing() const { return spacing_; }

	private:
		/// <summary>プローブの間隔(m)</summary>
		float spacing_ = 1.0f;
	};

}
