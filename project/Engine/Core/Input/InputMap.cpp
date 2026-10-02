#include "Engine/Core/Input/InputMap.h"
#include <algorithm>
#include <filesystem>
#include <json.hpp>
#include "Engine/Core/Input/Input.h"
#include "Engine/Core/Serialize/JsonSerializer.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/Core/App/MyWindow.h"

using namespace Core;
using namespace Math;


namespace {
	const std::filesystem::path kDirectory = "resource/Json/Input";
	const char* const kFileName = "InputMap.json";

	constexpr int32_t kMouseButtonCount = 8;
	constexpr float kTriggerMax = 255.0f;

	struct CodeName {
		uint16_t code;
		const char* name;
	};

	// ここに無いキーは "Key<番号>" で読み書きする
	constexpr CodeName kKeyNames[] = {
		{ DIK_ESCAPE, "Escape" },
		{ DIK_1, "1" }, { DIK_2, "2" }, { DIK_3, "3" }, { DIK_4, "4" }, { DIK_5, "5" },
		{ DIK_6, "6" }, { DIK_7, "7" }, { DIK_8, "8" }, { DIK_9, "9" }, { DIK_0, "0" },
		{ DIK_MINUS, "Minus" }, { DIK_EQUALS, "Equals" }, { DIK_BACK, "Backspace" }, { DIK_TAB, "Tab" },
		{ DIK_Q, "Q" }, { DIK_W, "W" }, { DIK_E, "E" }, { DIK_R, "R" }, { DIK_T, "T" },
		{ DIK_Y, "Y" }, { DIK_U, "U" }, { DIK_I, "I" }, { DIK_O, "O" }, { DIK_P, "P" },
		{ DIK_LBRACKET, "LBracket" }, { DIK_RBRACKET, "RBracket" }, { DIK_RETURN, "Enter" }, { DIK_LCONTROL, "LCtrl" },
		{ DIK_A, "A" }, { DIK_S, "S" }, { DIK_D, "D" }, { DIK_F, "F" }, { DIK_G, "G" },
		{ DIK_H, "H" }, { DIK_J, "J" }, { DIK_K, "K" }, { DIK_L, "L" },
		{ DIK_SEMICOLON, "Semicolon" }, { DIK_APOSTROPHE, "Apostrophe" }, { DIK_GRAVE, "Grave" },
		{ DIK_LSHIFT, "LShift" }, { DIK_BACKSLASH, "Backslash" },
		{ DIK_Z, "Z" }, { DIK_X, "X" }, { DIK_C, "C" }, { DIK_V, "V" }, { DIK_B, "B" }, { DIK_N, "N" }, { DIK_M, "M" },
		{ DIK_COMMA, "Comma" }, { DIK_PERIOD, "Period" }, { DIK_SLASH, "Slash" }, { DIK_RSHIFT, "RShift" },
		{ DIK_LMENU, "LAlt" }, { DIK_SPACE, "Space" }, { DIK_CAPITAL, "CapsLock" },
		{ DIK_F1, "F1" }, { DIK_F2, "F2" }, { DIK_F3, "F3" }, { DIK_F4, "F4" }, { DIK_F5, "F5" }, { DIK_F6, "F6" },
		{ DIK_F7, "F7" }, { DIK_F8, "F8" }, { DIK_F9, "F9" }, { DIK_F10, "F10" }, { DIK_F11, "F11" }, { DIK_F12, "F12" },
		{ DIK_NUMPAD0, "Num0" }, { DIK_NUMPAD1, "Num1" }, { DIK_NUMPAD2, "Num2" }, { DIK_NUMPAD3, "Num3" }, { DIK_NUMPAD4, "Num4" },
		{ DIK_NUMPAD5, "Num5" }, { DIK_NUMPAD6, "Num6" }, { DIK_NUMPAD7, "Num7" }, { DIK_NUMPAD8, "Num8" }, { DIK_NUMPAD9, "Num9" },
		{ DIK_MULTIPLY, "NumMultiply" }, { DIK_SUBTRACT, "NumMinus" }, { DIK_ADD, "NumPlus" }, { DIK_DECIMAL, "NumPeriod" },
		{ DIK_NUMPADENTER, "NumEnter" }, { DIK_DIVIDE, "NumDivide" },
		{ DIK_RCONTROL, "RCtrl" }, { DIK_RMENU, "RAlt" },
		{ DIK_UP, "Up" }, { DIK_DOWN, "Down" }, { DIK_LEFT, "Left" }, { DIK_RIGHT, "Right" },
		{ DIK_HOME, "Home" }, { DIK_END, "End" }, { DIK_PRIOR, "PageUp" }, { DIK_NEXT, "PageDown" },
		{ DIK_INSERT, "Insert" }, { DIK_DELETE, "Delete" },
	};

	constexpr CodeName kPadButtonNames[] = {
		{ XINPUT_GAMEPAD_DPAD_UP, "DPadUp" }, { XINPUT_GAMEPAD_DPAD_DOWN, "DPadDown" },
		{ XINPUT_GAMEPAD_DPAD_LEFT, "DPadLeft" }, { XINPUT_GAMEPAD_DPAD_RIGHT, "DPadRight" },
		{ XINPUT_GAMEPAD_START, "Start" }, { XINPUT_GAMEPAD_BACK, "Back" },
		{ XINPUT_GAMEPAD_LEFT_THUMB, "LThumb" }, { XINPUT_GAMEPAD_RIGHT_THUMB, "RThumb" },
		{ XINPUT_GAMEPAD_LEFT_SHOULDER, "LB" }, { XINPUT_GAMEPAD_RIGHT_SHOULDER, "RB" },
		{ XINPUT_GAMEPAD_A, "A" }, { XINPUT_GAMEPAD_B, "B" }, { XINPUT_GAMEPAD_X, "X" }, { XINPUT_GAMEPAD_Y, "Y" },
	};

	constexpr const char* kTriggerNames[] = { "LT", "RT" };
	constexpr const char* kStickNames[] = { "LStick", "RStick" };
	constexpr const char* kUnknownKeyPrefix = "Key";

	std::string KeyName(uint16_t code) {
		for (const CodeName& entry : kKeyNames) {
			if (entry.code == code) { return entry.name; }
		}
		return kUnknownKeyPrefix + std::to_string(code);
	}

	uint16_t KeyCode(const std::string& name) {
		for (const CodeName& entry : kKeyNames) {
			if (name == entry.name) { return entry.code; }
		}
		const size_t prefixLength = std::char_traits<char>::length(kUnknownKeyPrefix);
		if (name.size() > prefixLength && name.compare(0, prefixLength, kUnknownKeyPrefix) == 0) {
			return static_cast<uint16_t>(std::atoi(name.c_str() + prefixLength));
		}
		return 0;
	}

	std::string PadName(const InputBinding& binding) {
		if (binding.source == InputSource::PadTrigger) { return kTriggerNames[binding.code & 1]; }
		if (binding.source == InputSource::PadStick) { return kStickNames[binding.code & 1]; }
		for (const CodeName& entry : kPadButtonNames) {
			if (entry.code == binding.code) { return entry.name; }
		}
		return {};
	}

	void ReadPad(const std::string& name, InputBinding& binding) {
		for (uint16_t i = 0; i < 2; ++i) {
			if (name == kTriggerNames[i]) { binding.source = InputSource::PadTrigger; binding.code = i; return; }
			if (name == kStickNames[i]) { binding.source = InputSource::PadStick; binding.code = i; return; }
		}
		binding.source = InputSource::PadButton;
		for (const CodeName& entry : kPadButtonNames) {
			if (name == entry.name) { binding.code = entry.code; return; }
		}
	}

	InputBinding ReadBinding(const nlohmann::json& data) {
		InputBinding binding;
		if (data.contains("key")) {
			binding.source = InputSource::Key;
			binding.code = KeyCode(data["key"].get<std::string>());
		} else if (data.contains("mouse")) {
			binding.source = InputSource::MouseButton;
			binding.code = data["mouse"].get<uint16_t>();
		} else if (data.contains("pad")) {
			ReadPad(data["pad"].get<std::string>(), binding);
		}
		if (data.contains("scale") && data["scale"].is_array() && data["scale"].size() >= 2) {
			binding.scale = { data["scale"][0].get<float>(), data["scale"][1].get<float>() };
		}
		return binding;
	}

	bool IsPad(InputSource source) {
		return source == InputSource::PadButton || source == InputSource::PadTrigger || source == InputSource::PadStick;
	}

	struct Sample {
		bool isActive = false;
		Vector2 value;
	};

	Sample Read(const Input& input, const InputBinding& binding) {
		Sample sample;
		sample.value = binding.scale;
		switch (binding.source) {
		case InputSource::Key:
			sample.isActive = binding.code != 0 && input.PushKey(static_cast<uint8_t>(binding.code));
			break;
		case InputSource::MouseButton:
			sample.isActive = binding.code < kMouseButtonCount && input.IsPressMouse(binding.code);
			break;
		case InputSource::PadButton:
			sample.isActive = input.PressButton(static_cast<PadInput>(binding.code));
			break;
		case InputSource::PadTrigger: {
			const uint8_t raw = binding.code == 0 ? input.GetLTrigger() : input.GetRTrigger();
			sample.isActive = raw > Input::kTriggerThreshold;
			sample.value = binding.scale * (raw / kTriggerMax);
			break;
		}
		case InputSource::PadStick: {
			const Vector2 stick = binding.code == 0 ? input.GetLStick() : input.GetRStick();
			sample.isActive = stick.x != 0.0f || stick.y != 0.0f;
			sample.value = { stick.x * binding.scale.x, stick.y * binding.scale.y };
			break;
		}
		}
		return sample;
	}
}


InputMap* InputMap::GetInstance() {
	static InputMap instance;
	return &instance;
}

uint32_t InputMap::Register(const char* name, InputValueType type) {
	const uint32_t id = FindOrAdd(name);
	actions_[id].type = type;
	actions_[id].isDeclared = true;
	return id;
}

void InputMap::Load() {
	contexts_.clear();
	isRebinding_ = false;

	const nlohmann::json data = JsonSerializer::DeserializeJsonData((kDirectory / kFileName).string());
	if (!data.is_object() || !data.contains("contexts")) {
		return;
	}
	for (const auto& contextData : data["contexts"]) {
		InputContext context;
		context.name = contextData.value("name", std::string());
		if (contextData.contains("mappings")) {
			for (const auto& mappingData : contextData["mappings"]) {
				InputMapping mapping;
				mapping.action = FindOrAdd(mappingData.value("action", std::string()));
				mapping.binding = ReadBinding(mappingData);
				context.mappings.push_back(mapping);
			}
		}
		contexts_.push_back(std::move(context));
	}
}

void InputMap::Save() const {
	nlohmann::json contexts = nlohmann::json::array();
	for (const InputContext& context : contexts_) {
		nlohmann::json mappings = nlohmann::json::array();
		for (const InputMapping& mapping : context.mappings) {
			const Action& action = actions_[mapping.action];
			nlohmann::json mappingData;
			mappingData["action"] = action.name;
			switch (mapping.binding.source) {
			case InputSource::Key:
				mappingData["key"] = KeyName(mapping.binding.code);
				break;
			case InputSource::MouseButton:
				mappingData["mouse"] = mapping.binding.code;
				break;
			default:
				mappingData["pad"] = PadName(mapping.binding);
				break;
			}
			if (action.type == InputValueType::Axis2D) {
				mappingData["scale"] = { mapping.binding.scale.x, mapping.binding.scale.y };
			}
			mappings.push_back(std::move(mappingData));
		}
		nlohmann::json contextData;
		contextData["name"] = context.name;
		contextData["mappings"] = std::move(mappings);
		contexts.push_back(std::move(contextData));
	}
	nlohmann::json data;
	data["contexts"] = std::move(contexts);

	std::filesystem::create_directories(kDirectory);
	JsonSerializer::SerializeJsonData(data, (kDirectory / kFileName).string());
}

// 前フレームの状態を退避 → 有効なコンテキストの割り当てを足し込む → 使われた機器を覚える
void InputMap::Update() {
	const Input& input = *Input::GetInstance();
	const Vector2 lStick = input.GetLStick();
	const Vector2 rStick = input.GetRStick();
	const bool analog[kAnalogCount] = {
		input.IsLTriggerPressed(),
		input.IsRTriggerPressed(),
		lStick.x != 0.0f || lStick.y != 0.0f,
		rStick.x != 0.0f || rStick.y != 0.0f,
	};

	for (Action& action : actions_) {
		action.wasPressed = action.isPressed;
		action.isPressed = false;
		action.value = {};
	}

	if (isRebinding_) {
		InputBinding captured;
		if (input.TriggerKey(DIK_ESCAPE)) {
			isRebinding_ = false;
		} else if (Capture(input, analog, captured)) {
			InputBinding& binding = contexts_[rebindContext_].mappings[rebindMapping_].binding;
			const bool wasStick = binding.source == InputSource::PadStick;
			const bool isStick = captured.source == InputSource::PadStick;
			if (wasStick != isStick) {
				binding.scale = isStick ? Vector2{ 1.0f, 1.0f } : Vector2{ 1.0f, 0.0f };
			}
			binding.source = captured.source;
			binding.code = captured.code;
			isRebinding_ = false;
		}
	} else {
		bool isKeyUsed = false;
		bool isPadUsed = false;
		for (const InputContext& context : contexts_) {
			if (!context.isActive) {
				continue;
			}
			for (const InputMapping& mapping : context.mappings) {
				const Sample sample = Read(input, mapping.binding);
				if (!sample.isActive) {
					continue;
				}
				Action& action = actions_[mapping.action];
				action.isPressed = true;
				if (action.type == InputValueType::Axis2D) {
					action.value += sample.value;
				}
				(IsPad(mapping.binding.source) ? isPadUsed : isKeyUsed) = true;
			}
		}
		for (Action& action : actions_) {
			action.value.x = std::clamp(action.value.x, -1.0f, 1.0f);
			action.value.y = std::clamp(action.value.y, -1.0f, 1.0f);
		}
		if (isPadUsed) { isPadActive_ = true; }
		if (isKeyUsed) { isPadActive_ = false; }
	}

	std::copy(std::begin(analog), std::end(analog), std::begin(prevAnalog_));
}

bool InputMap::Press(uint32_t action) const {
	return actions_[action].isPressed;
}

bool InputMap::Trigger(uint32_t action) const {
	return actions_[action].isPressed && !actions_[action].wasPressed;
}

bool InputMap::Release(uint32_t action) const {
	return !actions_[action].isPressed && actions_[action].wasPressed;
}

Vector2 InputMap::Axis(uint32_t action) const {
	return actions_[action].value;
}

void InputMap::SetContextActive(const std::string& name, bool isActive) {
	for (InputContext& context : contexts_) {
		if (context.name == name) {
			context.isActive = isActive;
		}
	}
}

void InputMap::BeginRebind(size_t contextIndex, size_t mappingIndex) {
	if (contextIndex >= contexts_.size() || mappingIndex >= contexts_[contextIndex].mappings.size()) {
		return;
	}
	isRebinding_ = true;
	rebindContext_ = contextIndex;
	rebindMapping_ = mappingIndex;
}

std::string InputMap::BindingName(const InputBinding& binding) {
	switch (binding.source) {
	case InputSource::Key:
		return binding.code == 0 ? "---" : KeyName(binding.code);
	case InputSource::MouseButton:
		return "Mouse" + std::to_string(binding.code);
	default:
		return "Pad " + PadName(binding);
	}
}

void InputMap::DebugGUI() {
#ifdef _DEBUGMODE
	if (ImGui::Button("保存")) { Save(); }
	ImGui::SameLine();
	if (ImGui::Button("再読込")) { Load(); }
	ImGui::SameLine();
	ImGui::TextUnformatted(isPadActive_ ? "Pad" : "Keyboard");

	size_t removeContext = contexts_.size();
	for (size_t c = 0; c < contexts_.size(); ++c) {
		InputContext& context = contexts_[c];
		ImGui::PushID(static_cast<int>(c));
		ImGui::Checkbox("##active", &context.isActive);
		ImGui::SameLine();
		if (ImGui::CollapsingHeader(context.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
			size_t removeMapping = context.mappings.size();
			if (ImGui::BeginTable("mappings", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV)) {
				ImGui::TableSetupColumn("action", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("input", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("scale", ImGuiTableColumnFlags_WidthFixed, 140.0f);
				ImGui::TableSetupColumn("remove", ImGuiTableColumnFlags_WidthFixed, 24.0f);
				for (size_t m = 0; m < context.mappings.size(); ++m) {
					InputMapping& mapping = context.mappings[m];
					const Action& action = actions_[mapping.action];
					ImGui::PushID(static_cast<int>(m));
					ImGui::TableNextRow();

					ImGui::TableNextColumn();
					ImVec4 color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
					if (!action.isDeclared) { color = ImGuiTextWarn(); }
					if (action.isPressed) { color = ImGuiTextOk(); }
					ImGui::TextColored(color, "%s", action.name.c_str());

					ImGui::TableNextColumn();
					const bool isTarget = isRebinding_ && rebindContext_ == c && rebindMapping_ == m;
					const std::string label = isTarget ? "..." : BindingName(mapping.binding);
					if (ImGui::Button(label.c_str(), ImVec2(-FLT_MIN, 0.0f))) {
						if (isTarget) {
							CancelRebind();
						} else {
							BeginRebind(c, m);
						}
					}

					ImGui::TableNextColumn();
					if (action.type == InputValueType::Axis2D) {
						ImGui::SetNextItemWidth(-FLT_MIN);
						ImGui::DragFloat2("##scale", &mapping.binding.scale.x, 0.05f, -1.0f, 1.0f, "%.2f");
					}

					ImGui::TableNextColumn();
					if (ImGui::SmallButton("x")) { removeMapping = m; }

					ImGui::PopID();
				}
				ImGui::EndTable();
			}
			if (removeMapping < context.mappings.size()) {
				CancelRebind();
				context.mappings.erase(context.mappings.begin() + removeMapping);
			}

			if (ImGui::Button("割り当て追加")) { ImGui::OpenPopup("addMapping"); }
			if (ImGui::BeginPopup("addMapping")) {
				for (uint32_t a = 0; a < actions_.size(); ++a) {
					if (ImGui::Selectable(actions_[a].name.c_str())) {
						context.mappings.push_back(InputMapping{ a, InputBinding{ InputSource::Key, 0, Vector2{ 1.0f, 0.0f } } });
						BeginRebind(c, context.mappings.size() - 1);
					}
				}
				ImGui::EndPopup();
			}
			if (context.mappings.empty()) {
				ImGui::SameLine();
				if (ImGui::Button("コンテキスト削除")) { removeContext = c; }
			}
		}
		ImGui::PopID();
	}
	if (removeContext < contexts_.size()) {
		CancelRebind();
		contexts_.erase(contexts_.begin() + removeContext);
	}

	ImGui::Separator();
	static char contextName[32] = {};
	ImGui::SetNextItemWidth(160.0f);
	ImGui::InputText("##contextName", contextName, sizeof(contextName));
	ImGui::SameLine();
	if (ImGui::Button("コンテキスト追加") && contextName[0] != '\0') {
		InputContext context;
		context.name = contextName;
		contexts_.push_back(std::move(context));
		contextName[0] = '\0';
	}
#endif // _DEBUGMODE
}

uint32_t InputMap::FindOrAdd(const std::string& name) {
	for (uint32_t i = 0; i < actions_.size(); ++i) {
		if (actions_[i].name == name) {
			return i;
		}
	}
	Action action;
	action.name = name;
	actions_.push_back(std::move(action));
	return static_cast<uint32_t>(actions_.size() - 1);
}

bool InputMap::Capture(const Input& input, const bool analog[kAnalogCount], InputBinding& out) const {
	for (uint16_t key = 1; key < 256; ++key) {
		if (input.TriggerKey(static_cast<uint8_t>(key))) {
			out.source = InputSource::Key;
			out.code = key;
			return true;
		}
	}

#ifdef _DEBUGMODE
	// デバッグGUIの上のクリックは拾わない
	const bool canUseMouse = !ImGui::GetIO().WantCaptureMouse || MyWin::GetGameView().isHovered;
#else
	const bool canUseMouse = true;
#endif // _DEBUGMODE
	if (canUseMouse) {
		for (int32_t button = 0; button < kMouseButtonCount; ++button) {
			if (input.IsTriggerMouse(button)) {
				out.source = InputSource::MouseButton;
				out.code = static_cast<uint16_t>(button);
				return true;
			}
		}
	}

	for (const CodeName& entry : kPadButtonNames) {
		if (input.TriggerButton(static_cast<PadInput>(entry.code))) {
			out.source = InputSource::PadButton;
			out.code = entry.code;
			return true;
		}
	}
	for (size_t i = 0; i < kAnalogCount; ++i) {
		if (analog[i] && !prevAnalog_[i]) {
			out.source = i < 2 ? InputSource::PadTrigger : InputSource::PadStick;
			out.code = static_cast<uint16_t>(i & 1);
			return true;
		}
	}
	return false;
}
