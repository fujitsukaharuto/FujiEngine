#pragma once

namespace Math {

	/// <summary>
	/// イージング関数群
	/// </summary>
	/// <remarks>
	/// 全て t=0 で 0、t=1 で 1 を返す。t は呼び出し側で 0〜1 に収めること
	/// (Clamp01 を通すと安全)。Back / Elastic / Bounce は途中で 0〜1 を飛び出す
	/// </remarks>
	namespace Ease {

		//========================================================================*/
		//* Quad (2乗)
		float InQuad(float t);
		float OutQuad(float t);
		float InOutQuad(float t);

		//========================================================================*/
		//* Cubic (3乗)
		float InCubic(float t);
		float OutCubic(float t);
		float InOutCubic(float t);

		//========================================================================*/
		//* Sine (サインカーブ)
		float InSine(float t);
		float OutSine(float t);
		float InOutSine(float t);

		//========================================================================*/
		//* Back (行き過ぎてから戻る)
		float InBack(float t);
		float OutBack(float t);
		float InOutBack(float t);

		//========================================================================*/
		//* その他
		/// <summary>バネのように揺れて収まる</summary>
		float OutElastic(float t);
		/// <summary>地面で跳ねる</summary>
		float OutBounce(float t);
		/// <summary>両端がなめらかなS字。3t^2-2t^3</summary>
		float SmoothStep(float t);

	}
}
