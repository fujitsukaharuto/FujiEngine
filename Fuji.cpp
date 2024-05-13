#include "Fuji.h"

#include "MyWindow.h"
#include "DXCom.h"

namespace {
	MyWin* mywin_ = nullptr;
	DXCom* dxcom_ = nullptr;
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
	dxcom_->GetInstance();
	dxcom_->InitDX();

}

void Fuji::StartFrame()
{
	dxcom_->FirstFrame();
}

void Fuji::EndFrame()
{
	dxcom_->SetBarrier();
	dxcom_->ClearRTV();
	dxcom_->Command();
	dxcom_->LastFrame();
}
