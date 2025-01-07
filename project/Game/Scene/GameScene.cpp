#include "GameScene.h"
#include "ImGuiManager.h"
#include "GlobalVariables.h"
#include "FPSKeeper.h"

#include "Particle/ParticleManager.h"
#include "Scene/SceneManager.h"



GameScene::GameScene() {}

GameScene::~GameScene() {
}

void GameScene::Initialize() {
	Init();

	obj3dCommon.reset(new Object3dCommon());
	obj3dCommon->Initialize();

	black_ = std::make_unique<Sprite>();
	black_->Load("white2x2.png");
	black_->SetColor({ 0.0f,0.0f,0.0f,1.0f });
	black_->SetSize({ 1280.0f,720.0f });
	black_->SetAnchor({ 0.0f,0.0f });

	/*sphere = std::make_unique<Object3d>();
	sphere->CreateSphere();

	test_ = std::make_unique<TestBaseObj>();
	test_->Initialize();
	test_->name_ = "testObj";

	test2_ = std::make_unique<TestBaseObj>();
	test2_->Initialize();
	test2_->name_ = "testObj2";*/

	enemyManager_ = std::make_unique<EnemyManager>();
	enemyManager_->Initialize();

	cMane_ = std::make_unique<CollisionManager>();

	terrain = std::make_unique<Object3d>();
	terrain->Create("terrain.obj");
	terrain->transform.scale = { 8.0f,8.0f,8.0f };

	player_ = std::make_unique<Player>();
	player_->Initialize();

	followCamera_ = std::make_unique<FollowCamera>();
	followCamera_->Initialize();
	followCamera_->SetTarget(&player_->GetTrans());
}

void GameScene::Update() {

	cMane_->Reset();

#ifdef _DEBUG

	

#endif // _DEBUG


	dxCommon_->UpDate();

	BlackFade();

	enemyManager_->Update();
	player_->Update();

	followCamera_->Update();


	for (auto& enemy : enemyManager_->GetEnemyList()) {
		cMane_->AddCollider(enemy->GetCollider());
	}
	cMane_->AddCollider(player_->GetCollider());
	if (player_->GetIsAttack()) {
		cMane_->AddCollider(player_->GetColliderAttack());
	}
	cMane_->CheckAllCollision();

	ParticleManager::GetInstance()->Update();
}

void GameScene::Draw() {

#pragma region 背景描画


	dxCommon_->ClearDepthBuffer();
#pragma endregion


#pragma region 3Dオブジェクト
	obj3dCommon->PreDraw();

	enemyManager_->Draw();
	terrain->Draw();
	player_->Draw();

	ParticleManager::GetInstance()->Draw();

#ifdef _DEBUG
	enemyManager_->DrawCollider();
	player_->DrawCollider();
	player_->DrawColliderAttack();
#endif // _DEBUG
	Line3dDrawer::GetInstance()->Render();

#pragma endregion


#pragma region 前景スプライト

	dxCommon_->PreSpriteDraw();
	black_->Draw();

#pragma endregion

}

void GameScene::BlackFade() {
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
}

void GameScene::ApplyGlobalVariables() {
	
}
