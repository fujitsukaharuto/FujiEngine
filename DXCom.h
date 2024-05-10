#pragma once
#include <Windows.h>
#include <string>
#include <format>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>

#include "MyWindow.h"

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")



class DXCom
{
public:
	DXCom();
	~DXCom();

	DXCom* GetInstance();

	void InitDX();

private:

	void CreateDXGI();
	void SettingAdapter();


	IDXGIFactory7* dxgiFactory = nullptr;
	IDXGIAdapter4* useAdapter = nullptr;
	DXGI_ADAPTER_DESC3 adapterDesc{};

};
