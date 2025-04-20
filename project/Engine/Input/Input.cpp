#include "Input.h"
#include <stdexcept>

#include "MyWindow.h"



Input* Input::GetInstance() {
	static Input instance;
	return &instance;
}


void Input::Initialize() {

	HINSTANCE hInstance = GetModuleHandle(nullptr);
	hwnd_ = MyWin::GetInstance()->GetHwnd();

	// DirectInputの初期化
	if (FAILED(DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, reinterpret_cast<void**>(directInput_.GetAddressOf()), nullptr))) {
		throw std::runtime_error("Failed to create DirectInput object");
	}

	// キーボードの初期化
	if (FAILED(directInput_->CreateDevice(GUID_SysKeyboard, &keyboard_, NULL))) {
		throw std::runtime_error("Failed to create Keyboard object");
	}

	if (FAILED(keyboard_->SetDataFormat(&c_dfDIKeyboard))) {
		throw std::runtime_error("Failed to create DataFormat object");
	}

	if (FAILED(keyboard_->SetCooperativeLevel(hwnd_, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY))) {
		throw std::runtime_error("Failed to create CooperativeLevel object");
	}

	// マウスデバイスの作成
	if (FAILED(directInput_->CreateDevice(GUID_SysMouse, devMouse_.GetAddressOf(), nullptr))) {
		throw std::runtime_error("Failed to create mouse device");
	}

	// データフォーマットの設定
	if (FAILED(devMouse_->SetDataFormat(&c_dfDIMouse2))) {
		throw std::runtime_error("Failed to set mouse data format");
	}

	// 協調レベルの設定
	if (FAILED(devMouse_->SetCooperativeLevel(hwnd_, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE))) {
		throw std::runtime_error("Failed to set cooperative level");
	}

	// デバイスの取得
	devMouse_->Acquire();

	ZeroMemory(&pad_.state_, sizeof(XINPUT_STATE));
	ZeroMemory(&pad_.statePre_, sizeof(XINPUT_STATE));

}

void Input::Finalize() {
	directInput_.Reset();
	devMouse_.Reset();
	keyboard_.Reset();
}


void Input::Update() {
	KeyboardUpdate();

	MouseUpdate();

	GamepadUpdate();
}



/// Mouse-------------------------------------

bool Input::IsPressMouse(int32_t mouseNumber) const {
	return (mouse_.rgbButtons[mouseNumber] & 0x80) != 0;
}

bool Input::IsTriggerMouse(int32_t buttonNumber) const {
	return (mouse_.rgbButtons[buttonNumber] & 0x80) != 0 && (mousePre_.rgbButtons[buttonNumber] & 0x80) == 0;
}



/// Key---------------------------------------

bool Input::PushKey(uint8_t keyNumber) const {
	return key_[keyNumber] & 0x80;
}

bool Input::TriggerKey(uint8_t keyNumber) const {
	return (key_[keyNumber] & 0x80) && !(keyPre_[keyNumber] & 0x80);
}



/// GamePad-----------------------------------

bool Input::GetGamepadState(XINPUT_STATE& out) const {
	if (pad_.isConnected_) {
		out = pad_.state_;
		return true;
	}
	return false;
}

bool Input::GetGamepadStatePrevious(XINPUT_STATE& out) const {
	if (pad_.isConnected_) {
		out = pad_.statePre_;
		return true;
	}
	return false;
}

void Input::SetPadDeadZone(int32_t deadZoneL, int32_t deadZoneR) {
	pad_.deadZoneL_ = deadZoneL;
	pad_.deadZoneR_ = deadZoneR;
	//pad = pad_;
}

Vector2 Input::GetLStick() const {
	return ApplyDeadZone(pad_.state_.Gamepad.sThumbLX, pad_.state_.Gamepad.sThumbLY, pad_.deadZoneL_);
}

Vector2 Input::GetRStick() const {
	return ApplyDeadZone(pad_.state_.Gamepad.sThumbRX, pad_.state_.Gamepad.sThumbRY, pad_.deadZoneR_);
}

uint8_t Input::GetLTrigger() const {
	return pad_.state_.Gamepad.bLeftTrigger;
}

uint8_t Input::GetRTrigger() const {
	return pad_.state_.Gamepad.bRightTrigger;
}

bool Input::PressButton(PadInput padInput) const {
	return (pad_.state_.Gamepad.wButtons & static_cast<WORD>(padInput)) != 0;
}

bool Input::TriggerButton(PadInput padInput) const {
	return (!(pad_.statePre_.Gamepad.wButtons & static_cast<WORD>(padInput)) &&
		(pad_.state_.Gamepad.wButtons & static_cast<WORD>(padInput)));
}

bool Input::ReleaseButton(PadInput padInput) const {
	return ((pad_.statePre_.Gamepad.wButtons & static_cast<WORD>(padInput)) &&
		!(pad_.state_.Gamepad.wButtons & static_cast<WORD>(padInput)));
}






void Input::MouseUpdate() {
	mousePre_ = mouse_;
	// 現在のマウス状態を取得
	HRESULT hr = devMouse_->GetDeviceState(sizeof(DIMOUSESTATE2), &mouse_);
	if (FAILED(hr)) {
		// デバイスが失われた場合に再取得
		if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED) {
			devMouse_->Acquire();
		}
	}

	// マウスの位置を更新
	POINT p;
	GetCursorPos(&p);
	ScreenToClient(hwnd_, &p);
	mousePosition_.x = float(p.x);
	mousePosition_.y = float(p.y);
}

void Input::KeyboardUpdate() {
	keyPre_ = key_;
	// キーボードの入力情報取得
	HRESULT hr = keyboard_->GetDeviceState(sizeof(key_), key_.data());
	if (FAILED(hr)) {
		// 失敗の時再取得
		hr = keyboard_->Acquire();
		while (hr == DIERR_INPUTLOST) {
			hr = keyboard_->Acquire();
		}
	}
}

void Input::GamepadUpdate() {

	pad_.statePre_ = pad_.state_;

	DWORD dwResult = XInputGetState(0, &pad_.state_);
	pad_.isConnected_ = (dwResult == ERROR_SUCCESS);

	if (pad_.isConnected_) {
		// 左スティックのデッドゾーン計算結果をキャッシュ
		Vector2 currentLStick = ApplyDeadZone(pad_.state_.Gamepad.sThumbLX, pad_.state_.Gamepad.sThumbLY, pad_.deadZoneL_);
		Vector2 previousLStick = ApplyDeadZone(pad_.statePre_.Gamepad.sThumbLX, pad_.statePre_.Gamepad.sThumbLY, pad_.deadZoneL_);
		// 左スティックの移動状態を更新
		isLStickMoving_ = currentLStick.Length() != 0.0f;
		isPrevLStickMoving_ = previousLStick.Length() != 0.0f;

		// 右スティックのデッドゾーン計算結果をキャッシュ
		Vector2 currentRStick = ApplyDeadZone(pad_.state_.Gamepad.sThumbRX, pad_.state_.Gamepad.sThumbRY, pad_.deadZoneR_);
		Vector2 previousRStick = ApplyDeadZone(pad_.statePre_.Gamepad.sThumbRX, pad_.statePre_.Gamepad.sThumbRY, pad_.deadZoneR_);
		// 右スティックの移動状態を更新
		isRStickMoving_ = currentRStick.Length() != 0.0f;
		isPrevRStickMoving_ = previousRStick.Length() != 0.0f;
	}
}

Vector2 Input::ApplyDeadZone(int32_t x, int32_t y, int32_t deadZone) const {
	Vector2 stick;

	float magnitude = sqrtf(static_cast<float>(x) * static_cast<float>(x) + static_cast<float>(y) * static_cast<float>(y));
	if (magnitude < deadZone) {
		stick.x = 0.0f;
		stick.y = 0.0f;
	} else {
		stick.x = static_cast<float>(x);
		stick.y = static_cast<float>(y);
	}

	return stick;
}
