#include "Input.h"
#include <stdexcept>

#include "MyWindow.h"

void Input::Initialize()
{

	HINSTANCE hInstance = GetModuleHandle(nullptr);
	hwnd_ = MyWin::GetInstance()->GetHwnd();

	// DirectInputの初期化
	if (FAILED(DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, reinterpret_cast<void**>(directInput_.GetAddressOf()), nullptr))) {
		throw std::runtime_error("Failed to create DirectInput object");
	}

	// キーボードの初期化
	if (FAILED(directInput_->CreateDevice(GUID_SysKeyboard, &keyboard_, NULL)))
	{
		throw std::runtime_error("Failed to create Keyboard object");
	}

	if (FAILED(keyboard_->SetDataFormat(&c_dfDIKeyboard)))
	{
		throw std::runtime_error("Failed to create DataFormat object");
	}

	if (FAILED(keyboard_->SetCooperativeLevel(hwnd_, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY)))
	{
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
}

void Input::Update()
{
	KeyboardUpdate();

	MouseUpdate();
}

void Input::MouseUpdate()
{
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

void Input::KeyboardUpdate()
{
	keyPre_ = key_;
	// キーボードの入力情報取得
	HRESULT hr = keyboard_->GetDeviceState(sizeof(key_), key_.data());
	if (FAILED(hr))
	{
		// 失敗の時再取得
		hr = keyboard_->Acquire();
		while (hr == DIERR_INPUTLOST)
		{
			hr = keyboard_->Acquire();
		}
	}
}

Input* Input::GetInstance()
{
	static Input instance;
	return &instance;
}

const DIMOUSESTATE2& Input::GetAllMouse() const
{
	return mouse_;
}

bool Input::IsPressMouse(int32_t mouseNumber) const
{
	return (mouse_.rgbButtons[mouseNumber] & 0x80) != 0;
}

bool Input::IsTriggerMouse(int32_t buttonNumber) const
{
	return (mouse_.rgbButtons[buttonNumber] & 0x80) != 0 && (mousePre_.rgbButtons[buttonNumber] & 0x80) == 0;
}

const Vector2& Input::GetMousePosition() const
{
	return mousePosition_;
}

const std::array<BYTE, 256>& Input::GetAllKey()
{
	return key_;
}

bool Input::PushKey(uint8_t keyNumber) const
{
	return key_[keyNumber] & 0x80;
}

bool Input::TriggerKey(uint8_t keyNumber) const
{
	return (key_[keyNumber] & 0x80) && !(keyPre_[keyNumber] & 0x80);
}
