#include "Engine/Core/Input/InputAction.h"
#include "Engine/Core/Input/InputMap.h"

using namespace Core;
using namespace Math;


InputAction::InputAction(const char* name, InputValueType type) {
	id_ = InputMap::GetInstance()->Register(name, type);
}

bool InputAction::Press() const {
	return InputMap::GetInstance()->Press(id_);
}

bool InputAction::Trigger() const {
	return InputMap::GetInstance()->Trigger(id_);
}

bool InputAction::Release() const {
	return InputMap::GetInstance()->Release(id_);
}

Vector2 InputAction::Axis() const {
	return InputMap::GetInstance()->Axis(id_);
}
