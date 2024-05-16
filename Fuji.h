#pragma once
#include "DXCom.h"
#include "Vector2Matrix.h"

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

private:

	

};
