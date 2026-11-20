#pragma once
#include <array>
#include <map>
#include <string>
#include <json_fwd.hpp>
#include "Engine/GameObject/GameObject.h"

namespace Scene { struct Environment; }

namespace GameObject {

	/// <summary>
	/// ポストプロセスボリューム (UE の Post Process Volume)。カメラが箱に入ると、上書きする項目を環境へ混ぜる
	/// </summary>
	/// <remarks>
	/// 位置が箱の中心、拡縮が大きさ(m)、回転も効く。箱の外でも Blend radius までは距離に応じて薄く混ぜる。
	/// 複数あれば Priority の小さい順に重ね、大きい方が勝つ
	/// </remarks>
	class PostProcessVolume : public GameObject {
	public:

		/// <summary>配置データ1件から生成する。空なら何も上書きしない箱</summary>
		void Create(const nlohmann::json& data);
		/// <summary>配置データ1件分として書き出す</summary>
		nlohmann::json ToJson() const;

		void Draw(bool is = false) override;
		void ParameterGUI() override;

		/// <summary>カメラの位置での混ぜる割合 (0〜1)</summary>
		float GetWeight(const Math::Vector3& cameraPosition) const;
		/// <summary>上書きする項目を weight の割合で env へ混ぜる</summary>
		void Blend(Scene::Environment& env, float weight) const;
		int GetPriority() const { return priority_; }

	private:
		/// <summary>カメラから箱までの距離。中なら 0</summary>
		float DistanceToBox(const Math::Vector3& point) const;

	private:
		int priority_ = 0;
		/// <summary>箱の外のこの距離(m)まで、離れるほど薄く混ぜる</summary>
		float blendRadius_ = 1.0f;
		float blendWeight_ = 1.0f;
		/// <summary>箱に関係なくどこでも効く</summary>
		bool isUnbound_ = false;
		/// <summary>上書きする項目。キーは Environment の JSON のキー</summary>
		std::map<std::string, std::array<float, 3>> overrides_;
	};

}
