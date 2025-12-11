#include "GameScene.h"
#include "ImGuiManager.h"
#include "GlobalVariables.h"
#include "FPSKeeper.h"
#include "Game/GameObj/FollowCamera.h"
#include "Game/GameObj/Player/PlayerBullet.h"
#include "Engine/Model/ModelManager.h"

#include "Particle/ParticleManager.h"
#include "Scene/SceneManager.h"

using namespace Audio;
using namespace Core;
using namespace Graphics;
using namespace Math;


GameScene::GameScene() {}

GameScene::~GameScene() {
	player_->Finalize();
	AudioPlayer::GetInstance()->SoundStopWave(*bgm_);
}

void GameScene::Initialize() {

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

	LoadSceneLevelData("resource/Json/GameScene_position.json"); // ここで座標読み込むけど現在プレイヤー別で設定しているので直す

	player_->Initialize();
	player_->SetDXCom(dxcommon_);
	player_->SetLandingTime(startPlayerLandingTime_);

	boss_->Initialize();
	boss_->SetDXCom(dxcommon_);
	boss_->SetPlayer(player_.get());
	boss_->SetSatrtWait(startPlayerLandingTime_ + 60.0f);

	followCamera_ = std::make_unique<FollowCamera>();
	followCamera_->Initialize();
	followCamera_->SetTarget(&player_->GetTrans());
	followCamera_->SetTranslate(player_->GetLandingStartPos());
	followCamera_->PreRotateUpdate(boss_->GetDefoultPos());

	key_ = std::make_unique<Sprite>();
	key_->Load("key_beta.png");
	key_->SetAnchor({ 1.0f,1.0f });
	key_->SetPos({ MyWin::kWindowWidth, MyWin::kWindowHeight, 0.0f });
	key_->SetSize({ 400.0f, 300.0f });

	pad_ = std::make_unique<Sprite>();
	pad_->Load("keyPad_beta.png");
	pad_->SetAnchor({ 1.0f,1.0f });
	pad_->SetPos({ MyWin::kWindowWidth, MyWin::kWindowHeight, 0.0f });
	pad_->SetSize({ 400.0f, 300.0f });

	gameover_ = std::make_unique<Sprite>();
	gameover_->Load("gameover_beta.png");
	gameover_->SetAnchor({ 0.0f,0.0f });
	gameover_->SetSize({ MyWin::kWindowWidth, MyWin::kWindowHeight });

	gameoverSelector_ = std::make_unique<Sprite>();
	gameoverSelector_->Load("boal16x16.png");
	gameoverSelector_->SetPos(selectPointL_);
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

	ParticleManager::Load(field, "fieldParticle");
	bgm_ = &AudioPlayer::GetInstance()->SoundLoadWave("UrbanBGM_01.wav");

}

void GameScene::Update() {

	cMane_->Reset();
	PadSwitch();

#ifdef _DEBUG

	ApplyGlobalVariables();

#endif // _DEBUG

	if (!player_->GetIsGameOver()) {// GameOverかどうか
		if (boss_->GetIsStart()) {//ボスが召喚時
			player_->SetTargetPos(boss_->GetDefoultPos());
		} else {
			player_->SetTargetPos(boss_->GetBossCore()->GetCollider()->GetWorldPos());
		}
		player_->Update();

		if (boss_->GetIsStart()) {
			if (boss_->GetIsSummon()) {
				followCamera_->SetTargetSpeed(panSpeed_ * 0.1f);
				followCamera_->SetFollowSpeed(panSpeed_ * 2.0f);
				followCamera_->SetOffsetSoon(0.0f);
				followCamera_->SetOffset(boss_->GetCameraRang(), 30.0f);
				CameraManager::GetInstance()->GetCamera()->transform.rotate = boss_->GetSummonCameraRotate();
				CameraManager::GetInstance()->GetCamera()->transform.translate = summonCameraPos;
			} else {
				followCamera_->Update(boss_->GetDefoultPos());
			}
		} else {
			followCamera_->SetOffset(boss_->GetCameraRang(), 30.0f);
			followCamera_->SetFollowSpeed(boss_->GetCameraFollowSpeed());
			followCamera_->Update(boss_->GetBossCore()->GetWorldPos());
		}

		boss_->Update();
		if (!boss_->GetIsStart() && player_->GetIsStart()) {
			player_->SetIsStart(false);
			followCamera_->ResetTargetSpeed();
			followCamera_->ResetFollowSpeed();
			AudioPlayer::GetInstance()->SoundLoop(*bgm_, 0.025f);
		}
	} else {
		AudioPlayer::GetInstance()->SoundStopWave(*bgm_);
		GameoverUpdate();
	}
	ContinueUpdate();

	field.Emit();

#ifdef _DEBUG
	if (input_->TriggerKey(DIK_8)) {
		SoundData& soundData1 = audioPlayer_->SoundLoadWave("shot.wav");
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
		if (boss_->GetIsNowDush()) {
			cMane_->AddCollider(boss_->GetCollider());
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
		int beamCount = 0;
		for (auto& beam : boss_->GetBeam()->GetBeams()) {
			if (boss_->GetBeam()->GetIsLive() && boss_->GetBeam()->GetChangeTime() <= 0.0f) {
				cMane_->AddCollider(beam.collider.get());
				if (boss_->GetBeam()->GetStep() == BeamStep::RotateBeam) {
					beamCount++;
				}
				if (beamCount > 0) {
					break;
				}
			}
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
	if (isPadDraw_) {
		pad_->Draw();
	} else {
		key_->Draw();
	}
	if (isGameover_) {
		gameover_->Draw();
		gameoverSelector_->Draw();
	}
	//test->Draw();
	if (blackTime_ != 0.0f) {
		black_->Draw();
	} else if (isContiuneFade_ || isGameoverFade_) {
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
	if (isChangeFase_) {
		if (blackTime_ < blackLimmite_) {
			blackTime_ += FPSKeeper::DeltaTimeFrame();
			if (blackTime_ >= blackLimmite_) {
				blackTime_ = blackLimmite_;
			}
		} else {
			if (isBackTitle_) {
				ChangeScene("TITLE", 40.0f);
			} else {
				ChangeScene("RESULT", 40.0f);
			}
		}
		black_->SetColor({ 0.0f,0.0f,0.0f,Lerp(0.0f,1.0f,(1.0f / blackLimmite_ * blackTime_)) });
	} else {
		if (blackTime_ > 0.0f) {
			if (FPSKeeper::DeltaTimeFrame() < FPSKeeper::GetClampFrame()) {
				blackTime_ -= FPSKeeper::DeltaTimeFrame();
			}
			if (blackTime_ <= 0.0f) {
				blackTime_ = 0.0f;
			}
			black_->SetColor({ 0.0f,0.0f,0.0f,Lerp(0.0f,1.0f,(1.0f / blackLimmite_ * blackTime_)) });
		}
	}
#ifdef _DEBUG
	if (Input::GetInstance()->TriggerKey(DIK_0)) {
		if (blackTime_ == 0.0f) {
			isChangeFase_ = true;
		}
	}
#endif // _DEBUG
	if (boss_->GetIsClear()) {
		if (blackTime_ == 0.0f) {
			isChangeFase_ = true;
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
}

void GameScene::GameoverUpdate() {
	if (!isGameoverFade_ && gameoverFadeTime_ == 0.0f) {
		isGameoverFade_ = true;
	}

	if (isGameoverFade_) {
		gameoverFadeTime_ += FPSKeeper::DeltaTimeFrame();
		float v = std::fmodf(gameoverFadeTime_ / fadeBaseTime_, 2.0f);
		if (v <= 1.0f) {
			black_->SetColor({ 0.0f,0.0f,0.0f,v });
		} else {
			isGameover_ = true;
			black_->SetColor({ 0.0f,0.0f,0.0f,2.0f - v });
		}
		if (gameoverFadeTime_ > fadeBaseTime_ * 2.0f) {
			isGameoverFade_ = false;
			black_->SetColor({ 0.0f,0.0f,0.0f,0.0f });
		}
	}

	if (isGameover_ && !isGameoverFade_ && !isContiuneFade_ && !isChangeFase_) {
		if (selectPoint_ == 0) {
			if (input_->TriggerKey(DIK_SPACE) || input_->PressButton(PadInput::A)) {
				isRestartOnce_ = true;
				isContiuneFade_ = true;
				contiuneFadeTime_ = 0.0f;
			}
			if (input_->TriggerKey(DIK_D) || input_->PressButton(PadInput::Right)) {
				selectPoint_ = 1;
				gameoverSelector_->SetPos(selectPointR_);
			}
		} else {
			if (input_->TriggerKey(DIK_SPACE) || input_->PressButton(PadInput::A)) {
				if (blackTime_ == 0.0f) {
					isChangeFase_ = true;
					isBackTitle_ = true;
				}
			}
			if (input_->TriggerKey(DIK_A) || input_->PressButton(PadInput::Left)) {
				selectPoint_ = 0;
				gameoverSelector_->SetPos(selectPointL_);
			}
		}
	}
}

void GameScene::ContinueUpdate() {
	if (isContiuneFade_) {
		contiuneFadeTime_ += FPSKeeper::DeltaTimeFrame();
		float v = std::fmodf(contiuneFadeTime_ / fadeBaseTime_, 2.0f);
		if (v <= 1.0f) {
			black_->SetColor({ 0.0f,0.0f,0.0f,v });
		} else {
			isGameover_ = false;
			if (isRestartOnce_) {
				isRestartOnce_ = false;
				player_->ReStart();
				boss_->ReStart();
				followCamera_->ReStart(boss_->GetBossCore()->GetWorldPos());
			}
			black_->SetColor({ 0.0f,0.0f,0.0f,2.0f - v });
		}
		if (contiuneFadeTime_ > fadeBaseTime_ * 2.0f) {
			isContiuneFade_ = false;
			gameoverFadeTime_ = 0.0f;
			black_->SetColor({ 0.0f,0.0f,0.0f,0.0f });
		}
	}
}

void GameScene::PadSwitch() {
	Input* input = Input::GetInstance();
	Vector2 lstick = input->GetLStick();
	if (fabsf(lstick.x) > 0.01f || fabsf(lstick.y) > 0.01f) {
		isPadDraw_ = true;
	}

	if (input->PressButton(PadInput::A) || input->PressButton(PadInput::X) || input->PressButton(PadInput::B) || input->PressButton(PadInput::Y) || input->IsLTriggerPressed() || input->IsRTriggerPressed()) {
		isPadDraw_ = true;
	}

	if (input->TriggerKey(DIK_SPACE) || input->PushKey(DIK_K) || input->PushKey(DIK_J) || input->PushKey(DIK_A) || input->PushKey(DIK_D) || input->PushKey(DIK_W) || input->PushKey(DIK_S)) {
		isPadDraw_ = false;
	}
}
