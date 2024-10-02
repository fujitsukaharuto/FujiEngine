#include "Fuji.h"

#include "MyWindow.h"
#include "DXCom.h"
#include "ImGuiManager.h"
#include "DebugCamera.h"

namespace {
	MyWin* mywin_ = nullptr;
	DXCom* dxcom_ = nullptr;
	ImGuiManager* imguiManager_ = nullptr;
	Input* input_ = nullptr;
	Audio* audio_ = nullptr;
	DebugCamera* debugCamera_ = nullptr;
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

}

void Fuji::InitDX()
{
	mywin_ = MyWin::GetInstance();
	mywin_->Initialize();
	debugCamera_ = DebugCamera::GetInstance();
	debugCamera_->Initialize();
	dxcom_ = dxcom_->GetInstance();
	dxcom_->Initialize(mywin_);
	dxcom_->SetDebugCamera(debugCamera_);
	audio_ = Audio::GetInstance();
	audio_->Initialize();
	input_ = Input::GetInstance();
	input_->Initialize();
	imguiManager_ = imguiManager_->GetInstance();
	imguiManager_->Init(mywin_, dxcom_);
	dxcom_->SettingTexture();

}

void Fuji::StartFrame()
{
	input_->Update();
	imguiManager_->Begin();
	dxcom_->PreDraw();
}

void Fuji::EndFrame()
{
	imguiManager_->End();
	imguiManager_->Draw();
	dxcom_->PostDraw();
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
	debugCamera_->Update();
	dxcom_->UpDate();
}

void Fuji::GetKeyStateAll(BYTE* key)
{
	assert(key);
	const std::array<BYTE, 256>& keys = input_->GetAllKey();
	memcpy(key, keys.data(), sizeof(keys));
}

SoundData Fuji::SoundLoadWave(const char* filename)
{
	return audio_->SoundLoadWave(filename);
}

void Fuji::SoundUnload(SoundData* soundData)
{
	audio_->SoundUnload(soundData);
}

void Fuji::SoundPlayWave(const SoundData& soundData)
{
	audio_->SoundPlayWave(soundData);
}
