#include "GameScene.h"
#include "Engine/FujiEngine.h"
#include "Game/GameObj/FollowCamera.h"
#include "Game/GameObj/Player/PlayerBullet.h"

using namespace Audio;
using namespace Core;
using namespace Graphics;
using namespace Math;
using namespace Scene;
using namespace DXC;


GameScene::GameScene() {}

GameScene::~GameScene() {
	player_->Finalize();
	AudioPlayer::GetInstance()->SoundStopWave(*bgm_);
}

void GameScene::Initialize() {

#pragma region シーン遷移用
	black_ = std::make_unique<Sprite>();
	black_->Load("white2x2.png");
	black_->SetColor(Colors::Black);
	black_->SetSize({ MyWin::kWindowWidth,MyWin::kWindowHeight });
	black_->SetAnchor({ 0.0f,0.0f });
#pragma endregion

	InitGameObj();

	cMane_ = std::make_unique<CollisionManager>();

	ParticleManager::Load(field_, "fieldParticle");
	bgm_ = &AudioPlayer::GetInstance()->SoundLoadWave("UrbanBGM_01.wav");

}

void GameScene::Update() {

	cMane_->Reset();
	PadSwitch();

	if (!player_->GetIsGameOver()) {// GameOverかどうか
		if (boss_->GetIsStart()) {//ボスが召喚時
			player_->SetTargetPos(boss_->GetDefaultPos());
		} else {
			player_->SetTargetPos(boss_->GetBossCore()->GetCollider()->GetWorldPos());
		}
		player_->Update();

		if (boss_->GetIsStart()) {
			if (boss_->GetIsSummon()) {
				followCamera_->SetTargetSpeed(panSpeed_ * 0.1f);
				followCamera_->SetFollowSpeed(panSpeed_ * 2.0f);
				followCamera_->SetOffsetSoon(0.0f);
				followCamera_->SetOffset(boss_->GetCameraRange(), 30.0f);
				CameraManager::GetInstance()->GetCamera()->GetTransform().rotate = boss_->GetSummonCameraRotate();
				CameraManager::GetInstance()->GetCamera()->GetTransform().translate = summonCameraPos_;
			} else {
				followCamera_->Update(boss_->GetDefaultPos());
			}
		} else {
			followCamera_->SetOffset(boss_->GetCameraRange(), 30.0f);
			followCamera_->SetFollowSpeed(boss_->GetCameraFollowSpeed());
			followCamera_->Update(boss_->GetBossCore()->GetWorldPos());
		}

		boss_->Update();
		if (!boss_->GetIsStart() && player_->GetIsStart()) {// プレイヤーの開始処理
			player_->SetIsStart(false);
			followCamera_->ResetTargetSpeed();
			followCamera_->ResetFollowSpeed();
			AudioPlayer::GetInstance()->SoundLoop(*bgm_, 0.025f);
		}
		if (boss_->GetIsDamageLight()) {
			lightManager_->GetPointLight()->SetLightPos(boss_->GetDamageLightPos());
			lightManager_->GetPointLight()->SetAttenuationLight(boss_->GetLightTime(), boss_->GetLightIntensity());
		}
	} else {
		AudioPlayer::GetInstance()->SoundStopWave(*bgm_);
		GameOverUpdate();
	}
	ContinueUpdate();

	field_.Emit();

#ifdef _DEBUGMODE
	if (input_->TriggerKey(DIK_8)) {
		SoundData& soundData1 = audioPlayer_->SoundLoadWave("shot.wav");
		audioPlayer_->SoundPlayWave(soundData1);
	}
#endif // _DEBUG

	BlackFade();

	if (!player_->GetIsGameOver()) {
		CollisionUpdate();
	}

}

void GameScene::Draw() {
#pragma region 背景描画


#pragma endregion

#pragma region 3Dオブジェクト
	skybox_->Draw();

	surroundings_->Draw();
	terrain_->Draw();
	player_->Draw();

	boss_->Draw();

#pragma endregion

#pragma region 前景スプライト
	if (!player_->GetIsStart()) {
		if (isPadDraw_) {
			pad_->Draw();
		} else {
			key_->Draw();
		}
	}
	if (isGameOver_) {
		gameOver_->Draw();
		gameOverSelector_->Draw();
	}
	if (blackTime_ != 0.0f) {
		black_->Draw();
	} else if (isContinueFade_ || isGameOverFade_) {
		black_->Draw();
	}

#pragma endregion
}

void GameScene::DebugGUI() {
#ifdef _DEBUGMODE
	ImGui::Indent();

	followCamera_->DebugGUI();

	player_->DebugGUI();

	boss_->DebugGUI();

	if (ImGui::CollapsingHeader("terrain")) {
		terrain_->DebugGUI();
	}

	ImGui::Unindent();
#endif // _DEBUG
}

void GameScene::ParticleDebugGUI() {
#ifdef _DEBUGMODE
	ImGui::Indent();
	


	ImGui::Unindent();
#endif // _DEBUG
}

void GameScene::BlackFade() {
	if (isChangePhase_) {
		if (blackTime_ < blackLimit_) {
			blackTime_ += FPSKeeper::DeltaTimeFrame();
			if (blackTime_ >= blackLimit_) {
				blackTime_ = blackLimit_;
			}
		} else {
			if (isBackTitle_) {
				ChangeScene("TITLE", 40.0f);
			} else {
				ChangeScene("RESULT", 40.0f);
			}
		}
		black_->SetColor({ 0.0f,0.0f,0.0f,Lerp(0.0f,1.0f,(1.0f / blackLimit_ * blackTime_)) });
	} else {
		if (blackTime_ > 0.0f) {
			if (FPSKeeper::DeltaTimeFrame() < FPSKeeper::GetClampFrame()) {
				blackTime_ -= FPSKeeper::DeltaTimeFrame();
			}
			if (blackTime_ <= 0.0f) {
				blackTime_ = 0.0f;
			}
			black_->SetColor({ 0.0f,0.0f,0.0f,Lerp(0.0f,1.0f,(1.0f / blackLimit_ * blackTime_)) });
		}
	}
#ifdef _DEBUGMODE
	if (Input::GetInstance()->TriggerKey(DIK_0)) {
		if (blackTime_ == 0.0f) {
			isChangePhase_ = true;
		}
	}
#endif // _DEBUG
	if (boss_->GetIsClear()) {
		if (blackTime_ == 0.0f) {
			isChangePhase_ = true;
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

void GameScene::InitGameObj() {
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

	surroundings_ = std::make_unique<Object3d>();
	surroundings_->Create("surroundings.gltf");
	surroundings_->LoadTransformFromJson("surroundings_transform.json");
	surroundings_->SetColor(surroundingColor_);
	surroundings_->SetLightEnable(LightMode::kSpotLightON);

	player_ = std::make_unique<Player>();
	boss_ = std::make_unique<Boss>();

	LoadSceneLevelData("resource/Json/GameScene_position.json"); // ここで座標読み込むけど現在プレイヤー別で設定しているので直す

	player_->Initialize();// プレイヤー
	player_->SetDXCom(dxcommon_);
	player_->SetLandingTime(startPlayerLandingTime_);

	boss_->Initialize();// ボス
	boss_->SetDXCom(dxcommon_);
	boss_->SetPlayer(player_.get());
	boss_->SetStartWait(startPlayerLandingTime_ + 60.0f);

	followCamera_ = std::make_unique<FollowCamera>();
	followCamera_->Initialize();
	followCamera_->SetTarget(&player_->GetTrans());
	followCamera_->SetTranslate(player_->GetLandingStartPos());
	followCamera_->PreRotateUpdate(boss_->GetDefaultPos());

	key_ = std::make_unique<Sprite>();// キーボード入力
	key_->Load("key_beta.png");
	key_->SetAnchor({ 1.0f,1.0f });
	key_->SetPos({ MyWin::kWindowWidth, MyWin::kWindowHeight, 0.0f });
	key_->SetSize({ 400.0f, 300.0f });

	pad_ = std::make_unique<Sprite>();// パッド入力
	pad_->Load("keyPad_beta.png");
	pad_->SetAnchor({ 1.0f,1.0f });
	pad_->SetPos({ MyWin::kWindowWidth, MyWin::kWindowHeight, 0.0f });
	pad_->SetSize({ 400.0f, 300.0f });

	gameOver_ = std::make_unique<Sprite>();
	gameOver_->Load("gameover_beta.png");
	gameOver_->SetAnchor({ 0.0f,0.0f });
	gameOver_->SetSize({ MyWin::kWindowWidth, MyWin::kWindowHeight });

	gameOverSelector_ = std::make_unique<Sprite>();
	gameOverSelector_->Load("ball16x16.png");
	gameOverSelector_->SetPos(selectPointL_);
	gameOverSelector_->SetColor({ 0.7f, 0.7f, 0.1f, 1.0f });
	gameOverSelector_->SetSize({ 40.0f, 40.0f });
}

void GameScene::CollisionUpdate() {
	cMane_->AddCollider(player_->GetCollider());
	for (auto& bullet : player_->GetPlayerBullet()) {// プレイヤーの弾
		if (bullet->GetIsLive() && !bullet->GetIsCharge()) {
			cMane_->AddCollider(bullet->GetCollider());
		}
	}
	if (boss_->GetIsNowDush()) {// ダッシュ時の判定
		cMane_->AddCollider(boss_->GetCollider());
	}
	cMane_->AddCollider(boss_->GetCoreCollider());
	for (auto& wall : boss_->GetWalls()) {// ボスの攻撃Wave
		if (wall->GetIsLive()) {
			cMane_->AddCollider(wall->GetCollider());
		}
	}
	for (auto& arrow : boss_->GetArrows()) {// ボスの攻撃Arrow
		if (arrow->GetIsLive()) {
			cMane_->AddCollider(arrow->GetCollider());
		}
	}
	for (auto& ring : boss_->GetUnderRings()) {// ボスの攻撃Ring
		if (ring->GetIsLive()) {
			cMane_->AddCollider(ring->GetCollider());
		}
	}
	int beamCount = 0;
	for (auto& beam : boss_->GetBeam()->GetBeams()) {// ボスの攻撃Beam
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

void GameScene::GameOverUpdate() {
	if (!isGameOverFade_ && gameOverFadeTime_ == 0.0f) {
		isGameOverFade_ = true;
	}

	if (isGameOverFade_) {
		gameOverFadeTime_ += FPSKeeper::DeltaTimeFrame();
		float v = std::fmodf(gameOverFadeTime_ / fadeBaseTime_, 2.0f);
		if (v <= 1.0f) {
			black_->SetColor({ 0.0f,0.0f,0.0f,v });
		} else {
			isGameOver_ = true;
			black_->SetColor({ 0.0f,0.0f,0.0f,2.0f - v });
		}
		if (gameOverFadeTime_ > fadeBaseTime_ * 2.0f) {
			isGameOverFade_ = false;
			black_->SetColor({ 0.0f,0.0f,0.0f,0.0f });
		}
	}

	if (isGameOver_ && !isGameOverFade_ && !isContinueFade_ && !isChangePhase_) {
		if (selectPoint_ == 0) {
			if (input_->TriggerKey(DIK_SPACE) || input_->PressButton(PadInput::A)) {
				isRestartOnce_ = true;
				isContinueFade_ = true;
				continueFadeTime_ = 0.0f;
			}
			if (input_->TriggerKey(DIK_D) || input_->PressButton(PadInput::Right)) {
				selectPoint_ = 1;
				gameOverSelector_->SetPos(selectPointR_);
			}
		} else {
			if (input_->TriggerKey(DIK_SPACE) || input_->PressButton(PadInput::A)) {
				if (blackTime_ == 0.0f) {
					isChangePhase_ = true;
					isBackTitle_ = true;
				}
			}
			if (input_->TriggerKey(DIK_A) || input_->PressButton(PadInput::Left)) {
				selectPoint_ = 0;
				gameOverSelector_->SetPos(selectPointL_);
			}
		}
	}
}

void GameScene::ContinueUpdate() {
	if (isContinueFade_) {
		continueFadeTime_ += FPSKeeper::DeltaTimeFrame();
		float v = std::fmodf(continueFadeTime_ / fadeBaseTime_, 2.0f);
		if (v <= 1.0f) {
			black_->SetColor({ 0.0f,0.0f,0.0f,v });
		} else {
			isGameOver_ = false;
			if (isRestartOnce_) {
				isRestartOnce_ = false;
				player_->ReStart();
				boss_->ReStart();
				followCamera_->ReStart(boss_->GetBossCore()->GetWorldPos());
			}
			black_->SetColor({ 0.0f,0.0f,0.0f,2.0f - v });
		}
		if (continueFadeTime_ > fadeBaseTime_ * 2.0f) {
			isContinueFade_ = false;
			gameOverFadeTime_ = 0.0f;
			black_->SetColor({ 0.0f,0.0f,0.0f,0.0f });
		}
	}
}

void GameScene::PadSwitch() {
	Input* input = Input::GetInstance();
	Vector2 lStick = input->GetLStick();
	if (fabsf(lStick.x) > 0.01f || fabsf(lStick.y) > 0.01f) {
		isPadDraw_ = true;
	}

	if (input->PressButton(PadInput::A) || input->PressButton(PadInput::X) || input->PressButton(PadInput::B) || input->PressButton(PadInput::Y) || input->IsLTriggerPressed() || input->IsRTriggerPressed()) {
		isPadDraw_ = true;
	}

	if (input->TriggerKey(DIK_SPACE) || input->PushKey(DIK_K) || input->PushKey(DIK_J) || input->PushKey(DIK_A) || input->PushKey(DIK_D) || input->PushKey(DIK_W) || input->PushKey(DIK_S)) {
		isPadDraw_ = false;
	}
}
