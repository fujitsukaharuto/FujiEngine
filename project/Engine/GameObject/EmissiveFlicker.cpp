#include "EmissiveFlicker.h"
#include <cmath>
#include <cstdint>
#include <iterator>
#include <string>
#include <json.hpp>
#include "Engine/Core/Debug/ImGuiManager.h"

namespace GameObject {

	namespace {
		constexpr const char* kModeNames[] = { "None", "Flicker", "Blink", "Pulse" };
		constexpr float kTwoPi = 6.28318530718f;

		/// <summary>整数から 0〜1 の決まった乱数</summary>
		float Hash(int value) {
			uint32_t x = static_cast<uint32_t>(value) * 747796405u + 2891336453u;
			x = ((x >> ((x >> 28u) + 4u)) ^ x) * 277803737u;
			x = (x >> 22u) ^ x;
			return static_cast<float>(x & 0xFFFFFFu) / static_cast<float>(0x1000000);
		}
	}

	float EmissiveFlicker::Evaluate(float time) const {
		const float t = time * speed;
		const int seedKey = static_cast<int>(seed * 9973.0f);
		switch (mode) {
		case Mode::Flicker: {
			// 2秒ごとの区切りの3割ほどで、短い区切りのいくつかが消える
			const int window = static_cast<int>(std::floor(t * 0.5f + seed * 7.0f));
			if (Hash(window * 31 + seedKey) > 0.3f) {
				return 1.0f;
			}
			const int slot = static_cast<int>(std::floor(t * 14.0f));
			return Hash(slot * 17 + seedKey) < 0.45f ? 0.08f : 1.0f;
		}
		case Mode::Blink: {
			const float phase = t + seed;
			return (phase - std::floor(phase)) < 0.55f ? 1.0f : 0.05f;
		}
		case Mode::Pulse:
			return 0.55f + 0.45f * std::sin(kTwoPi * (t + seed));
		default:
			return 1.0f;
		}
	}

	EmissiveFlicker EmissiveFlicker::Read(const nlohmann::json& data) {
		EmissiveFlicker flicker;
		if (!data.contains("flicker") || !data["flicker"].is_object()) {
			return flicker;
		}
		const nlohmann::json& f = data["flicker"];
		const std::string name = f.value("mode", std::string("None"));
		for (int i = 0; i < static_cast<int>(std::size(kModeNames)); i++) {
			if (name == kModeNames[i]) {
				flicker.mode = static_cast<Mode>(i);
			}
		}
		flicker.speed = f.value("speed", flicker.speed);
		flicker.seed = f.value("seed", flicker.seed);
		return flicker;
	}

	void EmissiveFlicker::Write(nlohmann::json& data) const {
		if (!IsEnabled()) {
			return;
		}
		data["flicker"] = { { "mode", kModeNames[static_cast<int>(mode)] }, { "speed", speed }, { "seed", seed } };
	}

	bool EmissiveFlicker::GUI() {
		bool changed = false;
	#ifdef _DEBUGMODE
		int index = static_cast<int>(mode);
		if (ImGui::Combo("Flicker", &index, kModeNames, static_cast<int>(std::size(kModeNames)))) {
			mode = static_cast<Mode>(index);
			changed = true;
		}
		if (IsEnabled()) {
			changed |= ImGui::DragFloat("Flicker speed", &speed, 0.01f, 0.01f, 20.0f);
			changed |= ImGui::SliderFloat("Flicker seed", &seed, 0.0f, 1.0f);
		}
	#endif // _DEBUGMODE
		return changed;
	}

}
