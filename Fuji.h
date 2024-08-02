#pragma once
#include "DXCom.h"
#include "Vector2Matrix.h"
#include "Audio.h"
#include "Input.h"

#define DIRECTINPUT_VERSION 0x0800 // DirectInputのバージョン指定
#include <dinput.h>


class Fuji
{
public:
	Fuji()=default;
	~Fuji()=default;

	static float GetkWindowWidth();
	static float GetkWindowHeight();

	static int ProcessMessage();
	static void CreatWind();


	static void InitDX();
	static void StartFrame();
	static void EndFrame();
	static void End();

	static DXCom* GetDXComInstance();

	static void UpDateDxc();
	static void GetKeyStateAll(BYTE* key);

	static SoundData SoundLoadWave(const char* filename);
	static void SoundUnload(SoundData* soundData);
	static void SoundPlayWave(const SoundData& soundData);

private:

	

};
