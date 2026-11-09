#pragma once
#include <json_fwd.hpp>
#include "Engine/GameObject/GameObject.h"

namespace GameObject {

	/// <summary>
	/// 反射プローブの箱。位置が箱の中心 (焼く位置)、拡縮が大きさ(m)
	/// </summary>
	/// <remarks>回転は使わない</remarks>
	class ReflectionProbe : public GameObject {
	public:

		/// <summary>配置データ1件から生成する。空なら既定の大きさ</summary>
		void Create(const nlohmann::json& data);
		/// <summary>配置データ1件分として書き出す</summary>
		nlohmann::json ToJson() const;

		void Draw(bool is = false) override;
		void ParameterGUI() override;

		Math::Vector3 GetCenter() const { return transform_.translate; }
		Math::Vector3 GetSize() const;
		float GetBlendDistance() const { return blendDistance_; }
		float GetIntensity() const { return intensity_; }
		bool IsBoxProjection() const { return isBoxProjection_; }

	private:
		/// <summary>箱の端からこの距離で外へ溶かす(m)</summary>
		float blendDistance_ = 1.0f;
		float intensity_ = 1.0f;
		/// <summary>映った物を箱の壁にあるとみなして向きを直す</summary>
		bool isBoxProjection_ = true;
	};

}
