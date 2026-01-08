#include "TitleScene.h"
#include "ImGuiManager.h"
#include "ModelManager.h"
#include "GlobalVariables.h"
#include "CameraManager.h"
#include "FPSKeeper.h"
#include "Math/Random/Random.h"
#include "Model/Line3dDrawer.h"
#include "Particle/ParticleManager.h"
#include "Scene/SceneManager.h"
#include "Engine/Editor/CommandManager.h"

using namespace Core;
using namespace Graphics;
using namespace Math;


TitleScene::TitleScene() {}

TitleScene::~TitleScene() {
	ParticleManager::GetParticleCSEmitterSurface(0).SetEmit(false);
}

void TitleScene::Initialize() {

	obj3dCommon.reset(new Object3dCommon());
	obj3dCommon->Initialize();

	CameraManager::GetInstance()->GetCamera()->transform.rotate = { cameraStartRotateX_,0.0f,0.0f };
	CameraManager::GetInstance()->GetCamera()->transform.translate = { 0.0f, 5.0f, -30.0f };
	

	dxcommon_->GetOffscreenManager()->ResetPostEffect();
	dxcommon_->GetOffscreenManager()->AddPostEffect(PostEffectList::Bloom);

#pragma region シーン遷移用
	black_ = std::make_unique<Sprite>();
	black_->Load("white2x2.png");
	black_->SetColor(Colors::Black);
	black_->SetSize({ float(MyWin::kWindowWidth),float(MyWin::kWindowHeight) });
	black_->SetAnchor({ 0.0f,0.0f });
#pragma endregion

	skybox_ = std::make_unique<SkyBox>();
	skybox_->SetCommonResources(dxcommon_, SRVManager::GetInstance(), CameraManager::GetInstance()->GetCamera());
	skybox_->Initialize();
	skybox_->SetColor(skyBoxColor_);

	terrain_ = std::make_unique<AnimationModel>();
	terrain_->Create("ground.obj");
	terrain_->IsMirrorOBJ(true);
	terrain_->SetEnvironmentCoeff(0.3f);
	terrain_->SetTexture("grass.jpg");
	terrain_->SetColor(terrainColor_);

	space_ = std::make_unique<Sprite>();
	space_->Load("spaceKey.png");
	space_->SetPos({ 640.0f,500.0f,0.0f });
	space_->SetSize({ 256.0f,128.0f });

	title_ = std::make_unique<Sprite>();
	title_->Load("Title.png");
	title_->SetPos({ titleStartX_,250.0f,0.0f });
	title_->SetSize({ 968.0f,159.0f });

	player_ = std::make_unique<Player>();
	json playerData = JsonSerializer::DeserializeJsonData("resource/Json/Game_Player.json");
	player_->SetModelDataJson(playerData);
	player_->Initialize();
	TitleLoadPlayerPoint();
	player_->SettingTitleStartPosition(playerStart_, playerCenter_, playerEnd_);


	particleTest_ = std::make_unique<Object3d>();
	particleTest_->CreateSphere();
	particleTest_->SetColor(Colors::Transparent);

	cMane_ = std::make_unique<CollisionManager>();

	ParticleManager::Load(emit, "lightning");
	
	const float radius = 200.0f;
	const int division = 12;
	const float PI = 3.14159265359f;

	for (int i = 0; i < division; i++) {
		float angle = (2.0f * PI / division) * i;

		float x = radius * std::cos(angle);
		float z = radius * std::sin(angle);

		int emitNum = ParticleManager::GetInstance()->InitGPUEmitterSurface("PointyTower.obj");
		auto& emitterCS = ParticleManager::GetParticleCSEmitterSurface(emitNum);
		emitterCS.SetEmit(true);
		emitterCS.SetPos({ x,0.0f,z });
		emitterCS.data_.count = 4000;
		emitterCS.data_.radius = 9.0f;
		emitterCS.data_.colorMax = { 0.0f,0.8f,0.4f };
		emitterCS.data_.colorMin = { 0.0f,0.8f,0.4f };
		csEmitterNums_.push_back(emitNum);
	}

	int emitNum = ParticleManager::GetInstance()->InitGPUEmitter();
	auto& emitterCS = ParticleManager::GetSphereEmitter(emitNum);
	emitterCS.Load("skyStar");
	emitterCS.SetEmit(true);

}

void TitleScene::Update() {

	cMane_->Reset();

#ifdef _DEBUG



#endif // _DEBUG

	BlackFade();
	skybox_->Update();


	if (FPSKeeper::DeltaTimeFrame() < FPSKeeper::GetClampFrame()) {
		startTime_ -= FPSKeeper::DeltaTimeFrame();
	}
	if (startTime_ <= titleCanMoveTime_) {
		float titlemoveT = (std::max)(startTime_ / titleCanMoveTime_, 0.0f);
		float titlePosX = std::lerp(titleEmdX_, titleStartX_, powf(titlemoveT, 4.0f));
		title_->SetPos({ titlePosX,250.0f,0.0f });
	}
	float cameraT = (std::max)(startTime_ / startMaxTime_, 0.0f);
	float rotateX = std::lerp(cameraEndRotateX_, cameraStartRotateX_, cameraT);
	CameraManager::GetInstance()->GetCamera()->transform.rotate = { rotateX,0.0f,0.0f };
	player_->TitleUpdate(startTime_);

	auto& emitter = ParticleManager::GetSphereEmitter(0);
	emitter.data_.prevTranslate = emitter.data_.translate;
	emitter.data_.translate = particleTest_->GetWorldPos();

	cMane_->CheckAllCollision();

	ParticleManager::GetInstance()->Update();
}

void TitleScene::Draw() {

#pragma region 背景描画


	dxcommon_->ClearDepthBuffer();
#pragma endregion


#pragma region 3Dオブジェクト
	skybox_->Draw();

	obj3dCommon->PreDraw();
	terrain_->Draw();

	player_->TitleDraw();

	//particleTest_->Draw();

#ifdef _DEBUG
	if (!uiInvisible_) {
		if (startTime_ < 0.0f) {
			space_->Draw();
		}
		title_->Draw();
	}
#else
	if (startTime_ < 0.0f) {
		space_->Draw();
	}
	title_->Draw();
#endif // _DEBUG


#ifdef _DEBUG
	CommandManager::GetInstance()->Draw();
#endif // _DEBUG

	ParticleManager::GetInstance()->Draw();

#ifdef _DEBUG
	emit.DrawSize();
#endif // _DEBUG

	//cube_->SkeletonDraw();
	Line3dDrawer::GetInstance()->Render();

#pragma endregion

	//test
#pragma region 前景スプライト

	dxcommon_->PreSpriteDraw();
	if (blackTime != 0.0f) {
		black_->Draw();
	}

#pragma endregion
	ModelManager::GetInstance()->PickingDataCopy();
}

void TitleScene::DebugGUI() {
#ifdef _DEBUG
	ImGui::Checkbox("UI Invisible", &uiInvisible_);
	if (ImGui::TreeNode("PlayerMovePoint")) {
		ImGui::DragFloat3("StartPoint", &playerStart_.x, 0.01f);
		ImGui::DragFloat3("CenterPoint", &playerCenter_.x, 0.01f);
		ImGui::DragFloat3("EndPoint", &playerEnd_.x, 0.01f);
		if (ImGui::Button("Test")) {
			startTime_ = startMaxTime_;
		}ImGui::SameLine();
		if (ImGui::Button("Save##player3Point")) {
			TitleSavePlayerPoint();
		}
		ImGui::TreePop();
	}
	ImGui::Separator();
	ImGui::Indent();
	if (ImGui::CollapsingHeader("particleTest")) {
		particleTest_->DebugGUI();
	}
	skybox_->DebugGUI();
	if (ImGui::CollapsingHeader("terrain")) {
		terrain_->DebugGUI();
	}
	ImGui::Unindent();
#endif // _DEBUG
}

void TitleScene::ParticleDebugGUI() {
#ifdef _DEBUG
	ImGui::Indent();
	emit.DebugGUI();
	ImGui::Unindent();
#endif // _DEBUG
}

void TitleScene::BlackFade() {
	if (isChangeFase) {
		if (blackTime < blackLimmite) {
			blackTime += FPSKeeper::DeltaTimeFrame();
			if (blackTime >= blackLimmite) {
				blackTime = blackLimmite;
			}
		} else {
			if (!isParticleDebugScene_) {
				ChangeScene("GAME", 40.0f);
			} else {
				ChangeScene("PARTICLEDEBUG", 40.0f);
			}
		}
	} else {
		if (blackTime > 0.0f) {
			blackTime -= FPSKeeper::DeltaTimeFrame();
			if (blackTime <= 0.0f) {
				blackTime = 0.0f;
			}
		}
	}
	black_->SetColor({ 0.0f,0.0f,0.0f,Lerp(0.0f,1.0f,(1.0f / blackLimmite * blackTime)) });
	XINPUT_STATE pad;
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		if (blackTime == 0.0f) {
			isChangeFase = true;
		}
	} else if (Input::GetInstance()->GetGamepadState(pad)) {
		if (Input::GetInstance()->TriggerButton(PadInput::A)) {
			if (blackTime == 0.0f) {
				isChangeFase = true;
			}
		}
	}
#ifdef _DEBUG
	if (Input::GetInstance()->PushKey(DIK_RETURN) && Input::GetInstance()->PushKey(DIK_P) && Input::GetInstance()->PushKey(DIK_D) && Input::GetInstance()->TriggerKey(DIK_S)) {
		if (blackTime == 0.0f) {
			isChangeFase = true;
			isParticleDebugScene_ = true;
		}
	}
#endif // _DEBUG
}

void TitleScene::ApplyGlobalVariables() {
}

void TitleScene::TitleLoadPlayerPoint() {
	json data = JsonSerializer::DeserializeJsonData("resource/Json/Title/PlayerPoint.json");

	playerStart_ = Vector3(data["start"][0], data["start"][1], data["start"][2]);
	playerCenter_ = Vector3(data["center"][0], data["center"][1], data["center"][2]);
	playerEnd_ = Vector3(data["end"][0], data["end"][1], data["end"][2]);
}

void TitleScene::TitleSavePlayerPoint() {
#ifdef _DEBUG
	json data;

	data["start"] = { playerStart_.x,playerStart_.y,playerStart_.z };
	data["center"] = { playerCenter_.x,playerCenter_.y,playerCenter_.z };
	data["end"] = { playerEnd_.x,playerEnd_.y,playerEnd_.z };

	JsonSerializer::SerializeJsonData(data, "resource/Json/Title/PlayerPoint.json");
#endif // _DEBUG
}
