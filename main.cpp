#include "Fuji.h"
#include <Windows.h>
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"


#include "MatrixCalculation.h"



int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	Fuji::InitDX();



	SoundData soundData1 = Fuji::SoundLoadWave("resource/Alarm01.wav");



	BYTE keys[256] = { 0 };
	BYTE preKeys[256] = { 0 };

	//ウィンドウのxボタンが押されるまでループ
	while (Fuji::ProcessMessage() == 0)
	{
		Fuji::StartFrame();
		memcpy(preKeys, keys, 256);
		Fuji::GetKeyStateAll(keys);

		if (keys[DIK_0]&&!(preKeys[DIK_0]))
		{
			OutputDebugStringA("Hit 0\n");
			//Fuji::SoundPlayWave(soundData1);
		}

		Fuji::UpDateDxc();


		Fuji::EndFrame();
	}

	//OutputDebugStringA("Hello,DirectX!\n");
	//Log(ConvertString(std::format(L"WSTRING{}\n", wstringValue)));

	Fuji::SoundUnload(&soundData1);

	Fuji::End();
	return 0;
}

