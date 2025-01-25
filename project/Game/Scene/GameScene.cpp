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
	GlobalVariables::GetInstance()->LoadFiles();
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
	kikArea_ = std::make_unique<KikArea>();
	cMane_ = std::make_unique<CollisionManager>();
	enemySpawn_ = std::make_unique<EnemySpawn>();
	fieldBlockManager_ = std::make_unique<FieldBlockManager>();
	///-------------------------------------------------------------
	///　初期化
	///-------------------------------------------------------------- 
	obj3dCommon->Initialize();


#pragma region シーン遷移用
	black_ = std::make_unique<Sprite>();
	black_->Load("white2x2.png");
	black_->SetColor({ 0.0f,0.0f,0.0f,1.0f });
	black_->SetSize({ 1280.0f,950.0f });
	black_->SetAnchor({ 0.0f,0.0f });
#pragma endregion

	MenuInit();

	keyPaneru_ = std::make_unique<Sprite>();
	keyPaneru_->Load("key.png");
	keyPaneru_->SetAnchor({ 0.0f,0.0f });

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
	kikArea_->Initialize();
	enemySpawn_->Init();
	fieldBlockManager_->Initialize();
	///set
	enemyManager_->SetPlayer(player_.get());
	ufo_->SetEnemyManager(enemyManager_.get());
	enemySpawn_->SetEnemyManager(enemyManager_.get());

	fieldBlockManager_->AddFieldBlock();
	fieldBlockManager_->AddFieldBlock();
	fieldBlockManager_->AddFieldBlock();

	/*enemyManager_->FSpawn();*/
}

void GameScene::Update() {

	cMane_->Reset();

	ParamaterEdit();// パラメータエディター
	BlackFade();

	Menu();
	if (!isMenu_) {
		///-------------------------------------------------------------
		///　更新
		///-------------------------------------------------------------- 
		/// 
		enemySpawn_->Update();

		field_->Update();
		skydome_->Update();
		player_->Update();
		ufo_->Update();
		gameCamera_->Update();
		enemyManager_->Update();
		fieldBlockManager_->Update();
		kikArea_->Update();

	
		cMane_->AddCollider(ufo_->GetCollider());
		cMane_->AddCollider(player_->GetKikCollider());
		cMane_->AddCollider(kikArea_->GetWeakAreaCollider());
		cMane_->AddCollider(kikArea_->GetNormalAreaCollider());
		cMane_->AddCollider(kikArea_->GetMaxPowerArea());
		cMane_->AddCollider(kikArea_->GetSpecialAttackArea());
		cMane_->AddCollider(player_->GetCollider());
		for (auto& field : fieldBlockManager_->GetFieldBlocks()) {
			cMane_->AddCollider(field->GetCollider());
		}

		for (auto& enemy : enemyManager_->GetEnemies()) {
			cMane_->AddCollider(enemy->GetCollider());
		}
		cMane_->CheckAllCollision();


		ParticleManager::GetInstance()->Update();
	}
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
	fieldBlockManager_->Draw();
	player_->Draw();
	enemyManager_->Draw();
	ufo_->Draw();
	kikArea_->Draw();

	ufo_->UFOLightDraw();

	ParticleManager::GetInstance()->Draw();
	Line3dDrawer::GetInstance()->Render();

#pragma endregion


#pragma region 前景スプライト

	dxCommon_->PreSpriteDraw();

	keyPaneru_->Draw();
	if (isMenu_) {
		menuPaneru_->Draw();
		menuButton1_->Draw();
		menuButton2_->Draw();
	}
	if (blackTime != 0.0f) {
		black_->Draw();
	}

	///-------------------------------------------------------------
	///　前景スプライト描画
	///-------------------------------------------------------------- 


#pragma endregion

}

void GameScene::BlackFade() {
	/*XINPUT_STATE pad;*/

	if (isChangeFase) {
		if (blackTime < blackLimmite) {
			blackTime += FPSKeeper::DeltaTime();
			if (blackTime >= blackLimmite) {
				blackTime = blackLimmite;
			}
		}
		else {
			if (!isTitle_) {
				SceneManager::GetInstance()->ChangeScene("RESULT", 40.0f);
			}
			else {
				SceneManager::GetInstance()->ChangeScene("TITLE", 40.0f);
			}
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

	///遷移条件
	if (ufo_->GetIsDeath()) {
		isChangeFase = true;
	}
	/*if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
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
	}*/
}

void GameScene::Menu() {
	XINPUT_STATE pad;
	if (!isMenu_) {
		if (Input::GetInstance()->TriggerKey(DIK_ESCAPE)) {
			if (blackTime == 0.0f) {
				isMenu_ = true;
			}
		}
		else if (Input::GetInstance()->GetGamepadState(pad)) {
			if (Input::GetInstance()->TriggerButton(PadInput::Start)) {
				if (blackTime == 0.0f) {
					isMenu_ = true;
				}
			}
		}
	}
	else {
		// esc
		if (Input::GetInstance()->TriggerKey(DIK_ESCAPE)) {
			if (blackTime == 0.0f) {
				isMenu_ = false;
			}
		}
		else if (Input::GetInstance()->GetGamepadState(pad)) {
			if (Input::GetInstance()->TriggerButton(PadInput::Start)) {
				if (blackTime == 0.0f) {
					isMenu_ = false;
				}
			}
		}

		// セレクト
		if (Input::GetInstance()->TriggerKey(DIK_W)) {
			if (blackTime == 0.0f) {
				nowSelect_ = 1;
				menuButton1_->SetSize(buttonSizeMax_);
				menuButton2_->SetSize({ 300.0f,100.0f });
			}
		}
		else if (Input::GetInstance()->TriggerKey(DIK_S)) {
			if (blackTime == 0.0f) {
				nowSelect_ = 2;
				menuButton1_->SetSize({ 300.0f,100.0f });
				menuButton2_->SetSize(buttonSizeMax_);
			}
		}
		else if (Input::GetInstance()->GetGamepadState(pad)) {
			if (Input::GetInstance()->TriggerButton(PadInput::Up)) {
				if (blackTime == 0.0f) {
					nowSelect_ = 1;
					menuButton1_->SetSize(buttonSizeMax_);
					menuButton2_->SetSize({ 300.0f,100.0f });
				}
			}
		}
		else if (Input::GetInstance()->GetGamepadState(pad)) {
			if (Input::GetInstance()->TriggerButton(PadInput::Down)) {
				if (blackTime == 0.0f) {
					nowSelect_ = 2;
					menuButton1_->SetSize({ 300.0f,100.0f });
					menuButton2_->SetSize(buttonSizeMax_);
				}
			}
		}

		// 決定
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			if (blackTime == 0.0f) {
				if (nowSelect_ == 1) {
					isMenu_ = false;
				}
				else {
					isChangeFase = true;
					isTitle_ = true;
				}
			}
		}
		else if (Input::GetInstance()->GetGamepadState(pad)) {
			if (Input::GetInstance()->TriggerButton(PadInput::A)) {
				if (blackTime == 0.0f) {
					if (nowSelect_ == 1) {
						isMenu_ = false;
					}
					else {
						isChangeFase = true;
						isTitle_ = true;
					}
				}
			}
		}

	}

}

void GameScene::MenuInit() {

	menuPaneru_ = std::make_unique<Sprite>();
	menuPaneru_->Load("white2x2.png");
	menuPaneru_->SetColor({ 0.78f,0.78f,0.78f,0.4f });
	menuPaneru_->SetSize({ 1280.0f,950.0f });
	menuPaneru_->SetAnchor({ 0.0f,0.0f });

	menuButton1_ = std::make_unique<Sprite>();
	menuButton1_->Load("menu_button1.png");
	menuButton1_->SetColor({ 1.0f,1.0f,1.0f,1.0f });
	menuButton1_->SetSize({ 300.0f,100.0f });
	menuButton1_->SetAnchor({ 0.5f,0.5f });
	menuButton1_->SetPos({ 400.0f,350.0f,0.0f });

	menuButton2_ = std::make_unique<Sprite>();
	menuButton2_->Load("menu_button2.png");
	menuButton2_->SetColor({ 1.0f,1.0f,1.0f,1.0f });
	menuButton2_->SetSize({ 300.0f,100.0f });
	menuButton2_->SetAnchor({ 0.5f,0.5f });
	menuButton2_->SetPos({ 400.0f,600.0f,0.0f });

}


void GameScene::ParamaterEdit() {
#ifdef _DEBUG
	ImGui::Begin("ParamaterEditor");
	player_->AdjustParm();
	enemyManager_->AdjustParm();
	ufo_->AdjustParm();
	kikArea_->AdjustParm();
	fieldBlockManager_->AdjustParm();
	ImGui::End();
	enemySpawn_->DrawImGuiEditor();
	
#endif
}