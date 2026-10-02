#include "GameScene.h"
#include <cassert>
#include <json.hpp>
#include "Engine/FujiEngine.h"
#include "Engine/Graphics/Camera/FollowCamera.h"
#include "Game/GameObj/Player/PlayerBullet.h"
#include "Engine/Core/Input/Input.h"
#include "Engine/Core/Input/InputMap.h"
#include "Game/Input/GameActions.h"
#include "Engine/Audio/AudioPlayer.h"
#include "Engine/Graphics/Light/LightManager.h"
#include "Engine/Core/Time/FPSKeeper.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Particle/ParticleManager.h"
#include "Engine/Graphics/Particle/GPUParticle/GPUEmitter/SphereEmitter.h"

using namespace Audio;
using namespace Core;
using namespace Graphics;
using namespace Math;
using namespace Scene;
using namespace DXC;
using namespace Collision;


GameScene::GameScene() {}

GameScene::~GameScene() {
	player_->Finalize();
	AudioPlayer::GetInstance()->SoundStopWave(*bgm_);
}

void GameScene::Initialize() {

	InitGameObj();

	ParticleManager::Load(field_, "fieldParticle");

	// 空の星。発生間隔が長いので、最初の1回は待たずに出す
	int emitNum = ParticleManager::GetInstance()->InitGPUEmitter();
	auto& skyStar = ParticleManager::GetSphereEmitter(emitNum);
	skyStar.Load("skyStar");
	skyStar.SetEmit(true);
	skyStar.Emit();

	bgm_ =&AudioPlayer::GetInstance()->SoundLoadWave("UrbanBGM_01.wav");

}

void GameScene::Update() {

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
		// GetIsDamageLight() は読むと下りる立ち上がり検出。位置はそこで決め、明るさは毎フレーム送る
		if (boss_->GetIsDamageLight()) {
			lightManager_->GetPointLight()->SetLightPos(boss_->GetDamageLightPos());
			lightManager_->GetPointLight()->SetAttenuation(boss_->GetDamageLightRadius(), 1.0f);
		}
		lightManager_->GetPointLight()->SetIntensity(boss_->GetDamageLightIntensity());
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

	CheckSceneChange();

	// ゲームオーバー中は当たり判定ごと止める
	SetCollisionEnabled(!player_->GetIsGameOver());

	// 操作説明は開始演出が済んでから、使っている方だけ出す
	const bool isPadActive = InputMap::GetInstance()->IsPadActive();
	key_->SetVisible(!player_->GetIsStart() && !isPadActive);
	pad_->SetVisible(!player_->GetIsStart() && isPadActive);
	gameOver_->SetVisible(isGameOver_);
	gameOverSelector_->SetVisible(isGameOver_);
	flash_->SetVisible(isGameOverFade_ || isContinueFade_);

}

void GameScene::Draw() {
#pragma region 背景描画


#pragma endregion

#pragma region 3Dオブジェクト
	player_->Draw();

	boss_->Draw();

#pragma endregion

#pragma region 前景スプライト

#pragma endregion
}

void GameScene::DebugGUI() {
#ifdef _DEBUGMODE
	ImGui::Indent();

	followCamera_->DebugGUI();


	ImGui::Unindent();
#endif // _DEBUG
}

void GameScene::ParticleDebugGUI() {
#ifdef _DEBUGMODE
	ImGui::Indent();
	


	ImGui::Unindent();
#endif // _DEBUG
}

void GameScene::CheckSceneChange() {
	// 暗転・明転中は受け付けない
	if (IsFading()) {
		return;
	}

#ifdef _DEBUGMODE
	if (Input::GetInstance()->TriggerKey(DIK_0)) {
		ChangeScene("RESULT");
	}
#endif // _DEBUG
	if (boss_->GetIsClear()) {
		ChangeScene("RESULT");
	}
}

void GameScene::InitGameObj() {
	player_ = std::make_unique<Player>();
	boss_ = std::make_unique<Boss>();

	// 出現位置は配置データ(resource/Json/Level/GAME.json)の SpawnPoint
	ApplySpawnPoint(player_->GetTrans());

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

	// 操作説明・ゲームオーバー・明滅の絵は配置データ(resource/Json/Level/GAME.json)に置いてある
	key_ = level_.FindSprite("key");
	pad_ = level_.FindSprite("pad");
	gameOver_ = level_.FindSprite("gameOver");
	gameOverSelector_ = level_.FindSprite("gameOverSelector");
	flash_ = level_.FindSprite("flash");
	assert(key_ && pad_ && gameOver_ && gameOverSelector_ && flash_);
	gameOverSelector_->SetPos(selectPointL_);
}

void GameScene::GameOverUpdate() {
	if (!isGameOverFade_ && gameOverFadeTime_ == 0.0f) {
		isGameOverFade_ = true;
	}

	if (isGameOverFade_) {
		gameOverFadeTime_ += FPSKeeper::DeltaTimeFrame();
		float v = std::fmodf(gameOverFadeTime_ / fadeBaseTime_, 2.0f);
		if (v <= 1.0f) {
			flash_->SetColor({ 0.0f,0.0f,0.0f,v });
		} else {
			isGameOver_ = true;
			flash_->SetColor({ 0.0f,0.0f,0.0f,2.0f - v });
		}
		if (gameOverFadeTime_ > fadeBaseTime_ * 2.0f) {
			isGameOverFade_ = false;
			flash_->SetColor({ 0.0f,0.0f,0.0f,0.0f });
		}
	}

	if (isGameOver_ && !isGameOverFade_ && !isContinueFade_ && !IsFading()) {
		if (selectPoint_ == 0) {
			if (Game::Action::kDecide.Trigger()) {
				isRestartOnce_ = true;
				isContinueFade_ = true;
				continueFadeTime_ = 0.0f;
			}
			if (Game::Action::kSelectRight.Trigger()) {
				selectPoint_ = 1;
				gameOverSelector_->SetPos(selectPointR_);
			}
		} else {
			if (Game::Action::kDecide.Trigger()) {
				ChangeScene("TITLE");
			}
			if (Game::Action::kSelectLeft.Trigger()) {
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
			flash_->SetColor({ 0.0f,0.0f,0.0f,v });
		} else {
			isGameOver_ = false;
			if (isRestartOnce_) {
				isRestartOnce_ = false;
				player_->ReStart();
				boss_->ReStart();
				followCamera_->ReStart(boss_->GetBossCore()->GetWorldPos());
			}
			flash_->SetColor({ 0.0f,0.0f,0.0f,2.0f - v });
		}
		if (continueFadeTime_ > fadeBaseTime_ * 2.0f) {
			isContinueFade_ = false;
			gameOverFadeTime_ = 0.0f;
			flash_->SetColor({ 0.0f,0.0f,0.0f,0.0f });
		}
	}
}
