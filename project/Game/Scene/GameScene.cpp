#include "GameScene.h"
#include"Camera/CameraManager.h"

#include "Particle/ParticleManager.h"

#include "Scene/SceneManager.h"

#include "Model/Line3dDrawer.h"

#include<imgui.h>


GameScene::GameScene() {}

GameScene::~GameScene() {
}

void GameScene::Initialize() {
	Init();

	///-------------------------------------------------------------
	///　生成
	///-------------------------------------------------------------- 
	obj3dCommon = std::make_unique<Object3dCommon>();
	player_ = std::make_unique<Player>();
	field_ = std::make_unique<Field>();
	skydome_ = std::make_unique<SkyDome>();
	gameCamera_ = std::make_unique<GameCamera>();
	enemyManager_ = std::make_unique<EnemyManager>();
	ufo_= std::make_unique<UFO>();
	cMane_ = std::make_unique<CollisionManager>();
	///-------------------------------------------------------------
	///　初期化
	///-------------------------------------------------------------- 
	obj3dCommon->Initialize();


#pragma region シーン遷移用
	black_ = std::make_unique<Sprite>();
	black_->Load("white2x2.png");
	black_->SetColor({ 0.0f,0.0f,0.0f,1.0f });
	black_->SetSize({ 1280.0f,720.0f });
	black_->SetAnchor({ 0.0f,0.0f });
#pragma endregion

	obj3dCommon.reset(new Object3dCommon());
	obj3dCommon->Initialize();

	sphere = std::make_unique<Object3d>();
	sphere->CreateSphere();

	suzunne = std::make_unique<Object3d>();
	suzunne->Create("suzanne.obj");

	field_->Initialize();
	skydome_->Initialize();
	player_->Initialize();
	gameCamera_->Initialize();
	enemyManager_->Initialize();
	ufo_->Initialize();


	///set
	ufo_->SetEnemyManager(enemyManager_.get());
}

void GameScene::Update() {

	cMane_->Reset();
	
	ParamaterEdit();// パラメータエディター
	BlackFade();

	///-------------------------------------------------------------
	///　更新
	///-------------------------------------------------------------- 
	field_->Update();
	skydome_->Update();
	player_->Update();
	ufo_->Update();
	gameCamera_->Update();
	enemyManager_->Update();

	for (auto& enemy : enemyManager_->GetEnemies()) {
		cMane_->AddCollider(enemy->GetCollider());
		cMane_->AddCollider(enemy->GetJumpCollider());
	}
	cMane_->AddCollider(ufo_->GetCollider());
	cMane_->AddCollider(player_->GetWeakColliderCollider());
	cMane_->CheckAllCollision();
	

	ParticleManager::GetInstance()->Update();
	
}

void GameScene::Draw() {

#pragma region 背景描画

	///-------------------------------------------------------------
	///　背景描画
	///--------------------------------------------------------------

	dxCommon_->ClearDepthBuffer();
#pragma endregion


#pragma region 3Dオブジェクト
	obj3dCommon->PreDraw();

	///-------------------------------------------------------------
	///　モデル描画
	///-------------------------------------------------------------- 
	skydome_->Draw();
	field_->Draw();
	player_->Draw();
	enemyManager_->Draw();
	ufo_->Draw();
	

	ParticleManager::GetInstance()->Draw();
	Line3dDrawer::GetInstance()->Render();

#pragma endregion


#pragma region 前景スプライト

	dxCommon_->PreSpriteDraw();

	if (blackTime != 0.0f) {
		black_->Draw();
	}

	///-------------------------------------------------------------
	///　前景スプライト描画
	///-------------------------------------------------------------- 


#pragma endregion

}

void GameScene::BlackFade() {
	XINPUT_STATE pad;

	if (isChangeFase) {
		if (blackTime < blackLimmite) {
			blackTime += FPSKeeper::DeltaTime();
			if (blackTime >= blackLimmite) {
				blackTime = blackLimmite;
			}
		}
		else {
			SceneManager::GetInstance()->ChangeScene("RESULT", 40.0f);
		}
	}
	else {
		if (blackTime > 0.0f) {
			blackTime -= FPSKeeper::DeltaTime();
			if (blackTime <= 0.0f) {
				blackTime = 0.0f;
			}
		}
	}
	black_->SetColor({ 0.0f,0.0f,0.0f,Lerp(0.0f,1.0f,(1.0f / blackLimmite * blackTime)) });
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		if (blackTime == 0.0f) {
			isChangeFase = true;
		}
	}
	
	else if (Input::GetInstance()->GetGamepadState(pad)) {
		if (Input::GetInstance()->TriggerButton(PadInput::Y)) {
			if (blackTime == 0.0f) {
				isChangeFase = true;
			}
		}
	}
}


void GameScene::ParamaterEdit() {
	ImGui::Begin("ParamaterEditor");
	player_->AdjustParm();
	enemyManager_->AdjustParm();
	ufo_->AdjustParm();
	ImGui::End();

}