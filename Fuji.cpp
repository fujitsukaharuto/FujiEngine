#include "Fuji.h"

#include "MyWindow.h"
#include "DXCom.h"

namespace {
	MyWin* mywin_ = nullptr;
	DXCom* dxcom_ = nullptr;
}

float Fuji::GetkWindowWidth()
{
	return MyWin::kWindowWidth;
}

float Fuji::GetkWindowHeight()
{
	return MyWin::kWindowHeight;
}

int Fuji::ProcessMessage()
{
	return mywin_->ProcessMessage() ? 1 : 0;
}

void Fuji::CreatWind()
{
	mywin_ = MyWin::GetInstance();
	mywin_->CreatWind();
}

void Fuji::InitDX()
{
	dxcom_ = dxcom_->GetInstance();
	dxcom_->InitDX();

}

void Fuji::StartFrame()
{
	dxcom_->FirstFrame();
	dxcom_->SetBarrier();
	dxcom_->ClearRTV();
}

void Fuji::EndFrame()
{
	dxcom_->Command();
	dxcom_->LastFrame();
}

void Fuji::End()
{
	dxcom_->ReleaseData();
}

void Fuji::SetWVP(const Matrix4x4& world, const Matrix4x4& wvp)
{
	dxcom_->SetWVPData(world, wvp);
}

void Fuji::SetSpriteWVP(const Matrix4x4& world, const Matrix4x4& wvp)
{
	dxcom_->SetSpriteWVPData(world, wvp);
}
