#include "Fuji.h"

#include "MyWindow.h"
#include "DXCom.h"
#include "ImGuiManager.h"

namespace {
	MyWin* mywin_ = nullptr;
	DXCom* dxcom_ = nullptr;
	ImGuiManager* imguiManager_ = nullptr;
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
	mywin_ = MyWin::GetInstance();
	mywin_->CreatWind();
	dxcom_ = dxcom_->GetInstance();
	dxcom_->InitDX(mywin_);
	imguiManager_ = imguiManager_->GetInstance();
	imguiManager_->Init(mywin_, dxcom_);
	dxcom_->SettingTexture();


}

void Fuji::StartFrame()
{
	imguiManager_->Begin();
	dxcom_->SetBarrier();
	dxcom_->ClearRTV();
}

void Fuji::EndFrame()
{
	imguiManager_->End();
	dxcom_->Command();
	imguiManager_->Draw();
	dxcom_->LastFrame();
}

void Fuji::End()
{
	dxcom_->ReleaseData();
	imguiManager_->Fin();
}

DXCom* Fuji::GetDXComInstance()
{
	DXCom* instance = dxcom_->GetInstance();
	return instance;
}

void Fuji::UpDateDxc()
{
	dxcom_->UpDate();
}
