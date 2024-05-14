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

	static void SetWVP(const Matrix4x4& world, const Matrix4x4& wvp);
	static void SetSpriteWVP(const Matrix4x4& world, const Matrix4x4& wvp);

private:

	

};
