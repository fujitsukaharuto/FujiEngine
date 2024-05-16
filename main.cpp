#include "Fuji.h"
#include <Windows.h>
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"


#include "MatrixCalculation.h"



int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	Fuji::InitDX();

	//ウィンドウのxボタンが押されるまでループ
	while (Fuji::ProcessMessage() == 0)
	{
		Fuji::StartFrame();

		Fuji::UpDateDxc();

		Fuji::EndFrame();
	}

	//OutputDebugStringA("Hello,DirectX!\n");
	//Log(ConvertString(std::format(L"WSTRING{}\n", wstringValue)));



	Fuji::End();
	return 0;
}

