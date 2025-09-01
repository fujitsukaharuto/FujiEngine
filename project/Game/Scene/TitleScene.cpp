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


TitleScene::TitleScene() {}

TitleScene::~TitleScene() {
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
	black_->SetColor({ 0.0f,0.0f,0.0f,1.0f });
	black_->SetSize({ 1280.0f,720.0f });
	black_->SetAnchor({ 0.0f,0.0f });
#pragma endregion

	skybox_ = std::make_unique<SkyBox>();
	skybox_->SetCommonResources(dxcommon_, SRVManager::GetInstance(), CameraManager::GetInstance()->GetCamera());
	skybox_->Initialize();

	terrain_ = std::make_unique<AnimationModel>();
	terrain_->Create("ground.obj");
	terrain_->IsMirrorOBJ(true);
	terrain_->SetEnvironmentCoeff(0.3f);
	terrain_->SetTexture("grass.jpg");
	terrain_->transform.scale = { 1.0f,1.0f,1.0f };
	terrain_->SetUVScale({ 20.0f,20.0f }, { 0.0f,0.0f });

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
	playerStart_ = { -3.5f,10.0f,-26.0f };
	playerCenter_ = { -3.9f,3.0f,-25.0f };
	playerEnd_ = { 14.2f,6.0f,-14.0f };
	player_->SettingTitleStartPosition(playerStart_, playerCenter_, playerEnd_);


	particleTest_ = std::make_unique<Object3d>();
	particleTest_->CreateSphere();
	particleTest_->SetColor({ 0.0f,0.0f,0.0f,0.0f });

	/*cube_ = std::make_unique<AnimationModel>();
	cube_->Create("T_boss.gltf");
	cube_->LoadAnimationFile("T_boss.gltf");
	cube_->LoadTransformFromJson("boss_transform.json");

	animParentObj_ = std::make_unique<Object3d>();
	animParentObj_->Create("boss.obj");
	animParentObj_->SetAnimParent(cube_->GetJointTrans("mixamorig:LeftHandIndex1"));
	animParentObj_->SetNoneScaleParent(true);
	animParentObj_->LoadTransformFromJson("AnimParent_transform.json");*/

	cMane_ = std::make_unique<CollisionManager>();

	ParticleManager::Load(emit, "lightning");

}

void TitleScene::Update() {

	cMane_->Reset();

#ifdef _DEBUG



#endif // _DEBUG

	BlackFade();
	skybox_->Update();


	if (FPSKeeper::DeltaTime() < 2.2f) {
		startTime_ -= FPSKeeper::DeltaTime();
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


	csEmitterMoveTime_ += FPSKeeper::DeltaTime();
	int csSize = int(ParticleManager::GetParticleCSEmitterSize());
	for (int i = 0; i < csSize; i++) {
		if (i == 1) {
			float speed = 0.05f;       // 周期（回転スピード）
			float radius = 20.0f;      // 半径
			float angle = csEmitterMoveTime_ * speed + i * (std::numbers::pi_v<float>*2.0f) / float(csSize);

			ParticleManager::GetParticleCSEmitter(i).emitter->translate.x = std::cos(angle) * radius;
			ParticleManager::GetParticleCSEmitter(i).emitter->translate.y = 5.0f;
			ParticleManager::GetParticleCSEmitter(i).emitter->translate.z = std::sin(angle) * radius;
		} else {
			ParticleManager::GetParticleCSEmitter(i).emitter->prevTranslate = ParticleManager::GetParticleCSEmitter(i).emitter->translate;
			ParticleManager::GetParticleCSEmitter(i).emitter->translate = particleTest_->GetWorldPos();

			//ParticleManager::GetParticleCSEmitter(i).emitter->count = 11000;
			//ParticleManager::GetParticleCSEmitter(i).emitter->frequency = 1.0f;
			//ParticleManager::GetParticleCSEmitter(i).emitter->radius = 18.0f;
		}
	}

	//emit.Emit();

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

	//cube_->Draw();
	//animParentObj_->Draw();

	space_->Draw();
	title_->Draw();

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
	ImGui::Indent();
	if (ImGui::CollapsingHeader("particleTest")) {
		particleTest_->DebugGUI();
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
			blackTime += FPSKeeper::DeltaTime();
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
			blackTime -= FPSKeeper::DeltaTime();
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
