#pragma once
#include "Vector2.h"
#include <Windows.h>
#include <array>
#include <vector>
#include <wrl.h>

#include <XInput.h>
#define DIRECTINPUT_VERSION 0x0800 // DirectInputのバージョン指定
#include <dinput.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

class Input
{
public:
	void Initialize();

	void Update();

	void MouseUpdate();

	void KeyboardUpdate();

	static Input* GetInstance();

	const DIMOUSESTATE2& GetAllMouse() const;

	bool IsPressMouse(int32_t mouseNumber) const;

	bool IsTriggerMouse(int32_t buttonNumber) const;

	const Vector2& GetMousePosition() const;

	const std::array<BYTE, 256>& GetAllKey();

	bool PushKey(uint8_t keyNumber) const;

	bool TriggerKey(uint8_t keyNumber) const;

private:
	Input() = default;
	~Input() = default;
	Input(const Input&) = delete;
	const Input& operator=(const Input&) = delete;

	Microsoft::WRL::ComPtr<IDirectInput8> directInput_ = nullptr;
	Microsoft::WRL::ComPtr<IDirectInputDevice8> devMouse_ = nullptr;
	Microsoft::WRL::ComPtr<IDirectInputDevice8> keyboard_ = nullptr;
	DIMOUSESTATE2 mouse_;
	DIMOUSESTATE2 mousePre_;
	std::array<BYTE, 256> key_;
	std::array<BYTE, 256> keyPre_;
	HWND hwnd_;
	Vector2 mousePosition_;
};