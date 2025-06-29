#include "GameScene.h"
#include "ImGuiManager.h"
#include "GlobalVariables.h"
#include "FPSKeeper.h"
#include "Game/GameObj/FollowCamera.h"
#include "Game/GameObj/Player/PlayerBullet.h"

#include "Particle/ParticleManager.h"
#include "Scene/SceneManager.h"



GameScene::GameScene() {}

GameScene::~GameScene() {
}

void GameScene::Initialize() {

	GlobalVariables* globalvariables = GlobalVariables::GetInstance();
	const char* groupName = "Sphere";
	const char* groupName2 = "Fence";

	globalvariables->CreateGroup(groupName);
	globalvariables->AddItem(groupName, "parametar", spherePara);
	globalvariables->AddItem(groupName, "Position", spherevec);

	globalvariables->CreateGroup(groupName2);
	globalvariables->AddItem(groupName2, "parametar", fencePara);
	globalvariables->AddItem(groupName2, "Position", fencevec);

	obj3dCommon.reset(new Object3dCommon());
	obj3dCommon->Initialize();


#pragma region シーン遷移用
	black_ = std::make_unique<Sprite>();
	black_->Load("white2x2.png");
	black_->SetColor({ 0.0f,0.0f,0.0f,1.0f });
	black_->SetSize({ 1280.0f,720.0f });
	black_->SetAnchor({ 0.0f,0.0f });
#pragma endregion

	terrain = std::make_unique<Object3d>();
	terrain->Create("terrain.obj");
	terrain->transform.scale = { 30.0f,30.0f,30.0f };
	terrain->SetUVScale({ 25.0f,25.0f }, { 0.0f,0.0f });

	skybox_ = std::make_unique<SkyBox>();
	skybox_->SetCommonResources(dxcommon_, SRVManager::GetInstance(), CameraManager::GetInstance()->GetCamera());
	skybox_->Initialize();

	player_ = std::make_unique<Player>();
	player_->Initialize();

	boss_ = std::make_unique<Boss>();
	boss_->Initialize();
	boss_->SetPlayer(player_.get());

	followCamera_ = std::make_unique<FollowCamera>();
	followCamera_->Initialize();
	followCamera_->SetTarget(&player_->GetTrans());

	mate = std::make_unique<Material>();
	mate->SetTextureNamePath("grass.png");
	mate->CreateMaterial();
	mate->SetColor({ 1.0f,0.0f,0.0f,1.0f });

	gameover_ = std::make_unique<Sprite>();
	gameover_->Load("uvChecker.png");
	gameover_->SetAnchor({ 0.0f,0.0f });
	gameover_->SetSize({ 1280.0f, 720.0f });


	ApplyGlobalVariables();

	cMane_ = std::make_unique<CollisionManager>();

	emit.count_ = 3;
	emit.frequencyTime_ = 20.0f;
	emit.name_ = "animetest";
	emit.pos_ = { 0.0f,2.0f,0.0f };
	emit.animeData_.lifeTime = 40.0f;
	emit.RandomSpeed({ -0.1f,0.1f }, { -0.1f,0.1f }, { -0.1f,0.1f });
	emit.RandomTranslate({ -0.1f,0.1f }, { -0.1f,0.1f }, { -0.1f,0.1f });
}

void GameScene::Update() {

	cMane_->Reset();

#ifdef _DEBUG

	ApplyGlobalVariables();

#endif // _DEBUG

	if (!player_->GetIsGameOver()) {
		player_->SetTargetPos(boss_->GetBossCore()->GetWorldPos());
		player_->Update();

		followCamera_->Update(boss_->GetBossCore()->GetWorldPos());

		boss_->Update();
		if (!boss_->GetIsStart() && player_->GetIsStart()) {
			player_->SetIsStart(false);
		}
	} else {
		if (input_->TriggerKey(DIK_R)) {
			player_->ReStart();
			boss_->ReStart();
			followCamera_->ReStart(boss_->GetBossCore()->GetWorldPos());
		}
	}

	if (input_->TriggerKey(DIK_5)) {
		emit.BurstAnime();
	}
	if (input_->TriggerKey(DIK_8)) {
		SoundData& soundData1 = audioPlayer_->SoundLoadWave("xxx.wav");
		audioPlayer_->SoundPlayWave(soundData1);
	}
	if (input_->TriggerKey(DIK_9)) {
		SoundData& soundData1 = audioPlayer_->SoundLoadWave("xxx.wav");
		audioPlayer_->SoundStopWave(soundData1);

	}
 	if (input_->TriggerKey(DIK_7)) {
		SoundData& soundData2 = audioPlayer_->SoundLoadWave("mokugyo.wav");
		audioPlayer_->SoundPlayWave(soundData2);
	}

	skybox_->Update();
	BlackFade();

	if (!player_->GetIsGameOver()) {
		cMane_->AddCollider(player_->GetCollider());
		for (auto& bullet : player_->GetPlayerBullet()) {
			if (bullet->GetIsLive() && !bullet->GetIsCharge()) {
				cMane_->AddCollider(bullet->GetCollider());
			}
		}
		cMane_->AddCollider(boss_->GetCoreCollider());
		for (auto& wall : boss_->GetWalls()) {
			if (wall->GetIsLive()) {
				cMane_->AddCollider(wall->GetCollider());
			}
		}
		for (auto& ring : boss_->GetUnderRings()) {
			if (ring->GetIsLive()) {
				cMane_->AddCollider(ring->GetCollider());
			}
		}
		if (boss_->GetBeam()->GetIsLive()) {
			cMane_->AddCollider(boss_->GetBeam()->GetCollider());
		}
		cMane_->CheckAllCollision();
	}

	ParticleManager::GetInstance()->Update();
}

void GameScene::Draw() {
#pragma region 背景描画

	boss_->CSDispatch();

	dxcommon_->ClearDepthBuffer();
#pragma endregion


#pragma region 3Dオブジェクト
	skybox_->Draw();


	obj3dCommon->PreDraw();


	terrain->Draw();
	player_->Draw();

	boss_->Draw();

	ParticleManager::GetInstance()->Draw();

#ifdef _DEBUG

	Line3dDrawer::GetInstance()->Render();
#endif // _DEBUG
#pragma endregion


#pragma region 前景スプライト

	dxcommon_->PreSpriteDraw();
	if (player_->GetIsGameOver()) {
		gameover_->Draw();
	}
	//test->Draw();
	if (blackTime != 0.0f) {
		black_->Draw();
	}

#pragma endregion
}

void GameScene::DebugGUI() {
#ifdef _DEBUG
	ImGui::Indent();

	followCamera_->DebugGUI();

	player_->DebugGUI();

	boss_->DebugGUI();

	ImGui::Unindent();
#endif // _DEBUG
}

void GameScene::ParticleDebugGUI() {
#ifdef _DEBUG
	ImGui::Indent();
	
	emit.DebugGUI();

	ImGui::Unindent();
#endif // _DEBUG
}

void GameScene::BlackFade() {
	if (isChangeFase) {
		if (blackTime < blackLimmite) {
			blackTime += FPSKeeper::DeltaTime();
			if (blackTime >= blackLimmite) {
				blackTime = blackLimmite;
			}
		} else {
			ChangeScene("RESULT", 40.0f);
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
#ifdef _DEBUG
	if (Input::GetInstance()->TriggerKey(DIK_0)) {
		if (blackTime == 0.0f) {
			isChangeFase = true;
		}
	}
#endif // _DEBUG
	if (boss_->GetIsClear()) {
		if (blackTime == 0.0f) {
			isChangeFase = true;
		}
	}
}

void GameScene::ApplyGlobalVariables() {
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	const char* groupName = "Sphere";
	const char* groupName2 = "Fence";

	spherePara = globalVariables->GetFloatValue(groupName, "parametar");
	spherevec = globalVariables->GetVector3Value(groupName, "Position");

	fencePara = globalVariables->GetFloatValue(groupName2, "parametar");
	fencevec = globalVariables->GetVector3Value(groupName2, "Position");
}
