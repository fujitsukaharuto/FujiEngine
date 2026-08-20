#include "Easing.h"

#include <cmath>
#include "Engine/Math/Constants.h"

namespace Math {
	namespace Ease {

		namespace {
			// Back の行き過ぎ量。数値は Robert Penner の定番値
			constexpr float kBackOvershoot = 1.70158f;
			constexpr float kBackOvershootInOut = kBackOvershoot * 1.525f;
		}

		float InQuad(float t) { return t * t; }
		float OutQuad(float t) { return 1.0f - (1.0f - t) * (1.0f - t); }
		float InOutQuad(float t) {
			return t < 0.5f ? 2.0f * t * t : 1.0f - 2.0f * (1.0f - t) * (1.0f - t);
		}

		float InCubic(float t) { return t * t * t; }
		float OutCubic(float t) {
			const float u = 1.0f - t;
			return 1.0f - u * u * u;
		}
		float InOutCubic(float t) {
			if (t < 0.5f) { return 4.0f * t * t * t; }
			const float u = -2.0f * t + 2.0f;
			return 1.0f - u * u * u * 0.5f;
		}

		float InSine(float t) { return 1.0f - std::cos(t * kHalfPi); }
		float OutSine(float t) { return std::sin(t * kHalfPi); }
		float InOutSine(float t) { return -(std::cos(kPi * t) - 1.0f) * 0.5f; }

		float InBack(float t) {
			return (kBackOvershoot + 1.0f) * t * t * t - kBackOvershoot * t * t;
		}
		float OutBack(float t) {
			const float u = t - 1.0f;
			return 1.0f + (kBackOvershoot + 1.0f) * u * u * u + kBackOvershoot * u * u;
		}
		float InOutBack(float t) {
			if (t < 0.5f) {
				const float u = 2.0f * t;
				return (u * u * ((kBackOvershootInOut + 1.0f) * u - kBackOvershootInOut)) * 0.5f;
			}
			const float u = 2.0f * t - 2.0f;
			return (u * u * ((kBackOvershootInOut + 1.0f) * u + kBackOvershootInOut) + 2.0f) * 0.5f;
		}

		float OutElastic(float t) {
			if (t <= 0.0f) { return 0.0f; }
			if (t >= 1.0f) { return 1.0f; }
			constexpr float period = kTwoPi / 3.0f;
			return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * period) + 1.0f;
		}

		float OutBounce(float t) {
			constexpr float n1 = 7.5625f;
			constexpr float d1 = 2.75f;
			if (t < 1.0f / d1) {
				return n1 * t * t;
			} else if (t < 2.0f / d1) {
				const float u = t - 1.5f / d1;
				return n1 * u * u + 0.75f;
			} else if (t < 2.5f / d1) {
				const float u = t - 2.25f / d1;
				return n1 * u * u + 0.9375f;
			}
			const float u = t - 2.625f / d1;
			return n1 * u * u + 0.984375f;
		}

		float SmoothStep(float t) { return t * t * (3.0f - 2.0f * t); }

	}
}
