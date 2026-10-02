#pragma once
#include <cstdint>
#include "Engine/Math/Vector/Vector2.h"

namespace Core {

	/// <summary>
	/// 入力アクションが返す値の種類
	/// </summary>
	enum class InputValueType : uint8_t {
		Button,
		Axis2D,
	};

	/// <summary>
	/// 入力アクションのハンドル
	/// </summary>
	/// <remarks>inline const で宣言して使う。どの入力に繋ぐかは InputMap が持つ</remarks>
	class InputAction {
	public:

		explicit InputAction(const char* name, InputValueType type = InputValueType::Button);

		bool Press() const;
		bool Trigger() const;
		bool Release() const;

		/// <summary>Axis2D の値。各成分は -1 ～ 1</summary>
		Math::Vector2 Axis() const;

		uint32_t GetId() const { return id_; }

	private:

		uint32_t id_ = 0;

	};
}
