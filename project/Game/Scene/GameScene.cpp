#include "GameScene.h"
#include "ImGuiManager.h"
#include "GlobalVariables.h"
#include "FPSKeeper.h"
#include "Model/PlaneDrawer.h"
#include "Particle/ParticleManager.h"
#include "Scene/SceneManager.h"
#include "Light/PointLightManager.h"



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

	paneru = std::make_unique<Sprite>();
	paneru->Load("ui.png");
	paneru->SetColor({ 1.0f,1.0f,1.0f,1.0f });
	paneru->SetSize({ 1280.0f,720.0f });
	paneru->SetAnchor({ 0.0f,0.0f });

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
	terrain->Create("ground.obj");
	terrain->SetUVScale({ 15.0f,15.0f });
	terrain->transform.scale = { 3.0f,3.0f,3.0f };

	sky = std::make_unique<Object3d>();
	sky->Create("sky.obj");
	sky->SetLightEnable(LightMode::kLightNone);
	sky->transform.scale = { 1.0f,1.0f,1.0f };

	player_ = std::make_unique<Player>();
	player_->Initialize();

	lockOn_ = std::make_unique<LockOn>();
	lockOn_->Initialize();

	player_->SetLockOn(lockOn_.get());

	followCamera_ = std::make_unique<FollowCamera>();
	followCamera_->Initialize();
	followCamera_->SetTarget(&player_->GetTrans());
	followCamera_->SetLockOn(lockOn_.get());

}

void GameScene::Update() {

	cMane_->Reset();

#ifdef _DEBUG

	PointLightManager::GetInstance()->GetSpotLight(0)->Debug();

#endif // _DEBUG


	dxCommon_->UpDate();

	BlackFade();

	enemyManager_->GetEnemyList().remove_if([this](const std::unique_ptr<Enemy>& enemy) {
		if (!enemy->IsLive()) {
			// ロックオン対象が削除される場合に解除する
			if (lockOn_->GetTarget() == enemy.get()) {
				lockOn_->SetTarget(nullptr);
			}
			return true;
		}
		return false;;
		});

	player_->Update();

	PointLightManager::GetInstance()->GetSpotLight(0)->spotLightData_->position.x = player_->GetModel()->GetWorldPos().x;
	PointLightManager::GetInstance()->GetSpotLight(0)->spotLightData_->position.z = player_->GetModel()->GetWorldPos().z;

	enemyManager_->SetTargetPos(player_->GetModel()->GetWorldPos());
	enemyManager_->Update();

	lockOn_->Update(enemyManager_->GetEnemyList());
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
	sky->Draw();
	player_->Draw();

	player_->SlashDraw();

	ParticleManager::GetInstance()->Draw();

#ifdef _DEBUG
	enemyManager_->DrawCollider();
	player_->DrawCollider();
	player_->DrawColliderAttack();
#endif // _DEBUG
	PlaneDrawer::GetInstance()->Render();
	Line3dDrawer::GetInstance()->Render();

#pragma endregion


#pragma region 前景スプライト

	dxCommon_->PreSpriteDraw();
	paneru->Draw();
	lockOn_->Draw();
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
	if (enemyManager_->isClear) {
		if (blackTime == 0.0f) {
			isChangeFase = true;
		}
	}
}

void GameScene::ApplyGlobalVariables() {
	
}
