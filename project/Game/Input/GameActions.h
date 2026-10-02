#pragma once
#include "Engine/Core/Input/InputAction.h"

namespace Game {

	/// <summary>
	/// ゲームが使う入力アクション
	/// </summary>
	/// <remarks>名前は InputMap.json の action と一致していること</remarks>
	namespace Action {

		// --- プレイヤー操作 ---
		inline const Core::InputAction kMove{ "Move", Core::InputValueType::Axis2D };
		inline const Core::InputAction kJump{ "Jump" };
		inline const Core::InputAction kAvoid{ "Avoid" };
		inline const Core::InputAction kAttack{ "Attack" };

		// --- メニュー操作 ---
		inline const Core::InputAction kDecide{ "Decide" };
		inline const Core::InputAction kSelectLeft{ "SelectLeft" };
		inline const Core::InputAction kSelectRight{ "SelectRight" };

	}
}
