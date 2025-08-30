#include "GameScene.h"
#include "ImGuiManager.h"
#include "GlobalVariables.h"
#include "FPSKeeper.h"
#include "Game/GameObj/FollowCamera.h"
#include "Game/GameObj/Player/PlayerBullet.h"
#include "Engine/Model/ModelManager.h"

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

	terrain = std::make_unique<AnimationModel>();
	terrain->Create("ground.obj");
	terrain->IsMirrorOBJ(true);
	terrain->SetEnvironmentCoeff(0.3f);
	terrain->SetTexture("grass.jpg");
	terrain->transform.scale = { 1.0f,1.0f,1.0f };
	terrain->SetUVScale({ 20.0f,20.0f }, { 0.0f,0.0f });

	skybox_ = std::make_unique<SkyBox>();
	skybox_->SetCommonResources(dxcommon_, SRVManager::GetInstance(), CameraManager::GetInstance()->GetCamera());
	skybox_->Initialize();

	player_ = std::make_unique<Player>();
	boss_ = std::make_unique<Boss>();

	LoadSceneLevelData("resource/Json/GameScene_position.json");

	player_->Initialize();
	player_->SetDXCom(dxcommon_);

	boss_->Initialize();
	boss_->SetDXCom(dxcommon_);
	boss_->SetPlayer(player_.get());


	followCamera_ = std::make_unique<FollowCamera>();
	followCamera_->Initialize();
	followCamera_->SetTarget(&player_->GetTrans());

	key_ = std::make_unique<Sprite>();
	key_->Load("key_beta.png");
	key_->SetAnchor({ 1.0f,1.0f });
	key_->SetPos({ 1280.0f, 730.0f, 0.0f });
	key_->SetSize({ 400.0f, 300.0f });

	gameover_ = std::make_unique<Sprite>();
	gameover_->Load("gameover_beta.png");
	gameover_->SetAnchor({ 0.0f,0.0f });
	gameover_->SetSize({ 1280.0f, 720.0f });

	gameoverSelector_ = std::make_unique<Sprite>();
	gameoverSelector_->Load("boal16x16.png");
	gameoverSelector_->SetPos({ 240.0f, 500.0f, 0.0f });
	gameoverSelector_->SetColor({ 0.7f, 0.7f, 0.1f, 1.0f });
	gameoverSelector_->SetSize({ 40.0f, 40.0f });

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
		player_->SetTargetPos(boss_->GetBossCore()->GetCollider()->GetWorldPos());
		player_->Update();

		if (boss_->GetIsStart()) {
			followCamera_->Update(boss_->GetDefoultPos());
		} else {
			followCamera_->Update(boss_->GetBossCore()->GetWorldPos());
		}

		boss_->Update();
		if (!boss_->GetIsStart() && player_->GetIsStart()) {
			player_->SetIsStart(false);
		}
	} else {
		if (selectPoint_ == 0) {
			if (input_->TriggerKey(DIK_SPACE)) {
				player_->ReStart();
				boss_->ReStart();
				followCamera_->ReStart(boss_->GetBossCore()->GetWorldPos());
			}
			if (input_->TriggerKey(DIK_D)) {
				selectPoint_ = 1;
				gameoverSelector_->SetPos({ 800.0f,500.0f,0.0f });
			}
		} else {
			if (input_->TriggerKey(DIK_SPACE)) {
				if (blackTime == 0.0f) {
					isChangeFase = true;
					isBackTitle_ = true;
				}
			}
			if (input_->TriggerKey(DIK_A)) {
				selectPoint_ = 0;
				gameoverSelector_->SetPos({ 240.0f,500.0f,0.0f });
			}
		}
	}

#ifdef _DEBUG
	if (input_->TriggerKey(DIK_8)) {
		SoundData& soundData1 = audioPlayer_->SoundLoadWave("xxx.wav");
		audioPlayer_->SoundPlayWave(soundData1);
	}
#endif // _DEBUG

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
		for (auto& arrow : boss_->GetArrows()) {
			if (arrow->GetIsLive()) {
				cMane_->AddCollider(arrow->GetCollider());
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
	key_->Draw();
	if (player_->GetIsGameOver()) {
		gameover_->Draw();
		gameoverSelector_->Draw();
	}
	//test->Draw();
	if (blackTime != 0.0f) {
		black_->Draw();
	}

#pragma endregion
	ModelManager::GetInstance()->PickingDataCopy();
}

void GameScene::DebugGUI() {
#ifdef _DEBUG
	ImGui::Indent();

	followCamera_->DebugGUI();

	player_->DebugGUI();

	boss_->DebugGUI();

	if (ImGui::CollapsingHeader("terrain")) {
		terrain->DebugGUI();
	}

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
			if (isBackTitle_) {
				ChangeScene("TITLE", 40.0f);
			} else {
				ChangeScene("RESULT", 40.0f);
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

void GameScene::LoadSceneLevelData(const std::string& name) {
	BaseScene::LoadSceneLevelData(name);
	for (const auto& objJson : sceneData_["objects"]) {
		if (objJson.contains("objectType")) {
			if (objJson["objectType"] == "Normal") {
				
			} else if (objJson["objectType"] == "Player") {
				player_->SetModelDataJson(objJson);
			} else if (objJson["objectType"] == "Boss") {
				
			}
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
