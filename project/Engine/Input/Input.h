#pragma once
#include "Vector/Vector2.h"
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
	void Finalize();
	void Update();


	/// Mouse-------------------------------------

	/// <summary>
	/// マウスが押されているか
	/// </summary>
	/// <param name="mouseNumber">マウスボタン番号</param>
	/// <returns>bool</returns>
	bool IsPressMouse(int32_t mouseNumber) const;

	/// <summary>
	/// マウスがトリガーされたか
	/// </summary>
	/// <param name="buttonNumber">マウスボタン番号</param>
	/// <returns>bool</returns>
	bool IsTriggerMouse(int32_t buttonNumber) const;

	/// <summary>
	/// 全マウス情報を取得
	/// </summary>
	/// <returns></returns>
	const DIMOUSESTATE2& GetAllMouse() const { return mouse_; }

	/// <summary>
	/// マウスの位置を取得
	/// </summary>
	/// <returns>const Vector2</returns>
	const Vector2& GetMousePosition() const { return mousePosition_; }

	/// <summary>
	/// マウスホイールの回転量を取得
	/// </summary>
	/// <returns></returns>
	float GetWheel()const { return static_cast<float>(mouse_.lZ); }


	/// Key---------------------------------------

	/// <summary>
	/// キーが押されているか
	/// </summary>
	/// <param name="keyNumber">キー番号</param>
	/// <returns>bool</returns>
	bool PushKey(uint8_t keyNumber) const;

	/// <summary>
	/// キーがトリガーされているか
	/// </summary>
	/// <param name="keyNumber">キー番号</param>
	/// <returns></returns>
	bool TriggerKey(uint8_t keyNumber) const;

	/// <summary>
	/// 全キー情報を取得
	/// </summary>
	/// <returns></returns>
	const std::array<BYTE, 256>& GetAllKey() { return key_; }


	/// GamePad-----------------------------------

	/// <summary>
	/// ゲームパッドの状態を取得
	/// </summary>
	/// <returns></returns>
	XINPUT_STATE GetGamepadState() const { return pad_.state_; }

	/// <summary>
	/// ゲームパッドの状態を取得
	/// </summary>
	/// <param name="out"></param>
	/// <returns>bool</returns>
	bool GetGamepadState(XINPUT_STATE& out)const;

	/// <summary>
	/// 前回のゲームパッドの状態を取得
	/// </summary>
	/// <param name="out"></param>
	/// <returns>bool</returns>
	bool GetGamepadStatePrevious(XINPUT_STATE& out)const;

	/// <summary>
	/// パッドのデッドゾーンの設定
	/// </summary>
	/// <param name="deadZoneL">左スティックのデッドゾーン</param>
	/// <param name="deadZoneR">右スティックのデッドゾーン</param>
	void SetPadDeadZone(int32_t deadZoneL, int32_t deadZoneR);

	/// <summary>
	/// 左スティックを取得
	/// </summary>
	/// <returns>Vector2</returns>
	Vector2 GetLStick()const;

	/// <summary>
	/// 右スティックを取得
	/// </summary>
	/// <returns></returns>
	Vector2 GetRStick()const;

	/// <summary>
	/// 左トリガーを取得
	/// </summary>
	/// <returns></returns>
	uint8_t GetLTrigger()const;

	/// <summary>
	/// 右トリガーを取得
	/// </summary>
	/// <returns></returns>
	uint8_t GetRTrigger()const;

	/// <summary>
	/// ボタンが押されているか
	/// </summary>
	/// <param name="padInput">パッド入力</param>
	/// <returns>bool</returns>
	bool PressButton(PadInput padInput)const;
	
	/// <summary>
	/// ボタンがトリガーされたか
	/// </summary>
	/// <param name="padInput">パッド入力</param>
	/// <returns>bool</returns>
	bool TriggerButton(PadInput padInput)const;

	/// <summary>
	/// ボタンが離されたか
	/// </summary>
	/// <param name="padInput">パッド入力</param>
	/// <returns>bool</returns>
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