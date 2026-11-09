#pragma once
#include <json_fwd.hpp>

namespace GameObject {

	/// <summary>
	/// 発光やライトの明るさを時間で揺らす (ネオンのちらつき・点滅・明暗)
	/// </summary>
	/// <remarks>同じ種類・速さ・種なら同じ明るさになる</remarks>
	struct EmissiveFlicker {
		enum class Mode {
			None,		// 揺らさない
			Flicker,	// 切れかけのネオン。ときどき短く消える
			Blink,		// 一定の間隔で点いたり消えたり
			Pulse,		// ゆっくり明るくなったり暗くなったり
		};

		Mode mode = Mode::None;
		/// <summary>速さ。Blink と Pulse は1秒あたりの回数</summary>
		float speed = 1.0f;
		/// <summary>ずらし (0〜1)</summary>
		float seed = 0.0f;

		bool IsEnabled() const { return mode != Mode::None; }
		/// <summary>その時刻の明るさの倍率 (0〜1)</summary>
		float Evaluate(float time) const;

		/// <summary>配置データの "flicker" を読む。無ければ揺らさない</summary>
		static EmissiveFlicker Read(const nlohmann::json& data);
		/// <summary>揺らすときだけ "flicker" を書く</summary>
		void Write(nlohmann::json& data) const;
		/// <summary>編集欄。触ったら true</summary>
		bool GUI();
	};

}
