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

enum class PadInput : WORD {
	Up = XINPUT_GAMEPAD_DPAD_UP,
	Down = XINPUT_GAMEPAD_DPAD_DOWN,
	Left = XINPUT_GAMEPAD_DPAD_LEFT,
	Right = XINPUT_GAMEPAD_DPAD_RIGHT,

	Start = XINPUT_GAMEPAD_START,
	Back = XINPUT_GAMEPAD_BACK,

	LeftStick = XINPUT_GAMEPAD_LEFT_THUMB, LStick = XINPUT_GAMEPAD_LEFT_THUMB,
	RightStick = XINPUT_GAMEPAD_RIGHT_THUMB, RStick = XINPUT_GAMEPAD_RIGHT_THUMB,

	LeftShoulder = XINPUT_GAMEPAD_LEFT_SHOULDER,
	RightShoulder = XINPUT_GAMEPAD_RIGHT_SHOULDER,

	A = XINPUT_GAMEPAD_A,
	B = XINPUT_GAMEPAD_B,
	X = XINPUT_GAMEPAD_X,
	Y = XINPUT_GAMEPAD_Y
};

struct GamePad {
	Microsoft::WRL::ComPtr<IDirectInputDevice8> device_;
	int32_t deadZoneL_;
	int32_t deadZoneR_;
	XINPUT_STATE state_;
	XINPUT_STATE statePre_;
	bool isConnected_ = false;
};


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

	XINPUT_STATE GetGamepadState() const { return pad_.state_; }

	bool GetGamepadState(XINPUT_STATE& out)const;

	bool GetGamepadStatePrevious(XINPUT_STATE& out)const;

	void SetPadDeadZone(int32_t deadZoneL, int32_t deadZoneR);

	Vector2 GetLStick()const;
	Vector2 GetRStick()const;

	uint8_t GetLTrigger()const;
	uint8_t GetRTrigger()const;

	bool PressButton(PadInput padInput)const;
	bool TriggerButton(PadInput padInput)const;
	bool ReleaseButton(PadInput padInput)const;

private:

	void MouseUpdate();
	void KeyboardUpdate();
	void GamepadUpdate();


	Vector2 ApplyDeadZone(int32_t x, int32_t y, int32_t deadZone) const;

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

	GamePad pad_;
	bool isLStickMoving_ = false;
	bool isPrevLStickMoving_ = false;

	bool isRStickMoving_ = false;
	bool isPrevRStickMoving_ = false;

};