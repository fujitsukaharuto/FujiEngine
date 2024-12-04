#pragma once
#include "Vector2.h"
#include <Windows.h>
#include <array>
#include <vector>
#include <wrl.h>

#include <XInput.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "xinput.lib")



class Input {
public:

	Input() = default;
	~Input() = default;

public:

	static Input* GetInstance();

	void Initialize();

	void Update();




	/// Mouse-------------------------------------

	bool IsPressMouse(int32_t mouseNumber) const;
	bool IsTriggerMouse(int32_t buttonNumber) const;

	const DIMOUSESTATE2& GetAllMouse() const { return mouse_; }


	const Vector2& GetMousePosition() const { return mousePosition_; }

	float GetWheel()const { return static_cast<float>(mouse_.lZ); }



	/// Key---------------------------------------

	bool PushKey(uint8_t keyNumber) const;

	bool TriggerKey(uint8_t keyNumber) const;

	const std::array<BYTE, 256>& GetAllKey() { return key_; }




	/// GamePad-----------------------------------

	XINPUT_STATE GetGamepadState() const { return gamepadState_; }

	bool GetGamepadState(XINPUT_STATE& out)const;

	bool GetGamepadStatePrevious(XINPUT_STATE& out)const;



private:

	void MouseUpdate();

	void KeyboardUpdate();

	void GamepadUpdate();

private:

	Input(const Input&) = delete;
	const Input& operator=(const Input&) = delete;

private:

	Microsoft::WRL::ComPtr<IDirectInput8> directInput_ = nullptr;
	Microsoft::WRL::ComPtr<IDirectInputDevice8> devMouse_ = nullptr;
	Microsoft::WRL::ComPtr<IDirectInputDevice8> keyboard_ = nullptr;
	DIMOUSESTATE2 mouse_;
	DIMOUSESTATE2 mousePre_;
	std::array<BYTE, 256> key_;
	std::array<BYTE, 256> keyPre_;
	HWND hwnd_;
	Vector2 mousePosition_;
	XINPUT_STATE gamepadState_;
	XINPUT_STATE gamepadStatePrevious_;


};