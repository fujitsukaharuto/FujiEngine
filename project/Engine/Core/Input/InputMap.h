#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "Engine/Core/Input/InputAction.h"

namespace Core {

	class Input;

	/// <summary>
	/// 割り当てる入力の種類
	/// </summary>
	enum class InputSource : uint8_t {
		Key,
		MouseButton,
		PadButton,
		PadTrigger,
		PadStick,
	};

	/// <summary>
	/// 割り当てる入力1つ
	/// </summary>
	struct InputBinding {
		InputSource source = InputSource::Key;
		uint16_t code = 0;					// Key=DIK / MouseButton=番号 / PadButton=XInputのビット / 他=0:左 1:右
		Math::Vector2 scale{ 1.0f, 1.0f };	// Axis2D 用。スティックは成分ごとの倍率、それ以外は押している間に足す向き
	};

	/// <summary>
	/// アクションと入力の対応1つ
	/// </summary>
	struct InputMapping {
		uint32_t action = 0;
		InputBinding binding;
	};

	/// <summary>
	/// 対応のまとまり。まとめて有効・無効を切り替える
	/// </summary>
	struct InputContext {
		std::string name;
		bool isActive = true;
		std::vector<InputMapping> mappings;
	};

	/// <summary>
	/// 入力アクションへの割り当てを持ち、毎フレーム値を求める
	/// </summary>
	/// <remarks>割り当ては resource/Json/Input/InputMap.json に持つ</remarks>
	class InputMap {
	public:

		static InputMap* GetInstance();

		/// <summary>アクションを登録して id を返す。同じ名前は同じ id になる</summary>
		uint32_t Register(const char* name, InputValueType type);

		void Load();
		void Save() const;

		/// <summary>Input::Update の後に呼ぶ</summary>
		void Update();

		bool Press(uint32_t action) const;
		bool Trigger(uint32_t action) const;
		bool Release(uint32_t action) const;
		Math::Vector2 Axis(uint32_t action) const;

		void SetContextActive(const std::string& name, bool isActive);

		/// <summary>最後に使われたのがパッドか。操作説明の出し分けに使う</summary>
		bool IsPadActive() const { return isPadActive_; }

		//========================================================================*/
		//* キーコンフィグ

		const std::vector<InputContext>& GetContexts() const { return contexts_; }
		const std::string& GetActionName(uint32_t action) const { return actions_[action].name; }

		/// <summary>次に押された入力を、指定の対応へ割り当てる</summary>
		/// <remarks>待っている間はどのアクションも反応しない。Escape で取り消す</remarks>
		void BeginRebind(size_t contextIndex, size_t mappingIndex);
		void CancelRebind() { isRebinding_ = false; }
		bool IsRebinding() const { return isRebinding_; }

		/// <summary>表示用の入力名</summary>
		static std::string BindingName(const InputBinding& binding);

		void DebugGUI();

	private:

		struct Action {
			std::string name;
			InputValueType type = InputValueType::Button;
			bool isDeclared = false;	// コードに InputAction がある。false は JSON にだけ残っている名前
			Math::Vector2 value;
			bool isPressed = false;
			bool wasPressed = false;
		};

		// 押し始めを取るための、トリガーとスティックの状態。並びは LT, RT, LStick, RStick
		static constexpr size_t kAnalogCount = 4;

		uint32_t FindOrAdd(const std::string& name);
		bool Capture(const Input& input, const bool analog[kAnalogCount], InputBinding& out) const;

	private:

		InputMap() = default;
		InputMap(const InputMap&) = delete;
		const InputMap& operator=(const InputMap&) = delete;

	private:

		std::vector<Action> actions_;
		std::vector<InputContext> contexts_;
		bool isPadActive_ = false;

		bool isRebinding_ = false;
		size_t rebindContext_ = 0;
		size_t rebindMapping_ = 0;
		bool prevAnalog_[kAnalogCount] = {};

	};
}
