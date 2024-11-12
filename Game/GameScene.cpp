#include "GameScene.h"
#include "ImGuiManager.h"
#include "GlobalVariables.h"
#include "Line3dDrawer.h"
#include "FPSKeeper.h"

#include "ParticleManager.h"



GameScene::GameScene() {}

GameScene::~GameScene() {
	delete nightSky;
	delete nightWater;
	for (auto suzunneModel : suzunnes) {
		delete suzunneModel;
	}
	delete terrain;
	delete scoreArea;
	delete player_;
	delete enemyManager_;

}

void GameScene::Initialize() {
	Init();

	obj3dCommon.reset(new Object3dCommon());
	obj3dCommon->Initialize();

	nightSky = new Object3d();
	nightSky->Create("nightSky.obj");
	nightSky->transform.scale = { 6.0f,6.0f,6.0f };

	nightWater = new Object3d();
	nightWater->Create("nightWater.obj");
	nightWater->transform.scale = { 600.0f,600.0f,600.0f };
	nightWater->transform.translate = { 0.0f,-7.0f,0.0f };

	float addDis = 1.0f;
	for (int i = 0; i < 3; i++) {

		Object3d* newModel = new Object3d();
		newModel->Create("suzanne.obj");
		newModel->transform.translate.x += addDis;
		newModel->transform.translate.z += addDis;
		newModel->transform.rotate.y = 3.14f;
		suzunnes.push_back(newModel);
		addDis += 0.5f;

	}


	terrain = new Object3d();
	terrain->Create("terrain.obj");


	scoreArea = new Sprite();
	scoreArea->Load("scoreArea.png");
	scoreArea->SetSize({ 350.0f,200.0f });
	scoreArea->SetPos({ 1100.0f,650.0f,0.0f });

	soundData1 = audio_->SoundLoadWave("resource/xxx.wav");
	soundData2 = audio_->SoundLoadWave("resource/mokugyo.wav");

	emit.count = 3;
	emit.frequencyTime = 20.0f;
	emit.name = "animetest";
	emit.pos = { 0.0f,2.0f,0.0f };
	emit.animeData.lifeTime = 40.0f;
	emit.RandomSpeed({ -0.1f,0.1f }, { -0.1f,0.1f }, { -0.1f,0.1f });
	emit.RandomTranslate({ -0.1f,0.1f }, { -0.1f,0.1f }, { -0.1f,0.1f });
	emit.grain.transform.scale = { 1.0f,1.0f,1.0f };

	editor.Initialize();

	player_ = new Player();
	player_->Initialize();

	enemyManager_ = new EnemyManager();
	enemyManager_->Initialize();


}

void GameScene::Update() {

	editor.Update();
	enemyManager_->Update();

#ifdef _DEBUG


	ImGui::Begin("fps");
	float fps = 1.0f / FPSKeeper::BaseTime();
	ImGui::Text("%f", fps);
	ImGui::End();


#endif // _DEBUG


	if (input_->TriggerKey(DIK_8)) {
		audio_->SoundPlayWave(soundData1);
		emit.BurstAnime();
	}
	if (input_->TriggerKey(DIK_9)) {
		audio_->SoundStopWave(soundData1);

	}
	if (input_->TriggerKey(DIK_7)) {
		audio_->SoundPlayWave(soundData2);
	}

	dxCommon_->UpDate();


	float rotaSpeed = 0.1f;
	for (auto suzunneModel : suzunnes) {
		suzunneModel->transform.rotate.x += rotaSpeed * FPSKeeper::DeltaTime();
		//suzunneModel->transform.translate = Random::GetVector3({ -4.0f,4.0f }, { -4.0f,4.0f }, { -4.0f,4.0f });
		rotaSpeed += 0.05f;
	}



	player_->Update();
	enemyManager_->EnemyUpdate();


	Vector3 bulletPos = player_->GetCenterBullet();
	Vector3 bulletEndPos= player_->GetEndBullet();

	for (auto& enemy : enemyManager_->GetEnemyList()) {
		Vector3 enemypos = enemy->GetCentarPos();

		if (enemy->GetLive()) {
			if (IsLineCollisionSphere(bulletPos, bulletEndPos, enemypos, 1.0f)) {
				enemy->SetLive(false);
				score += 60;
			}
		}
	}

#ifdef _DEBUG

	ImGui::Begin("Score");
	ImGui::Text("score : %d", score);
	ImGui::End();

#endif // _DEBUG



	/*emit.Emit();*/
	ParticleManager::GetInstance()->Update();

}

void GameScene::Draw() {

#pragma region 背景描画


	dxCommon_->ClearDepthBuffer();
#pragma endregion


#pragma region 3Dオブジェクト
	obj3dCommon->PreDraw();
	nightWater->Draw();
	nightSky->Draw();

	player_->Draw();
	enemyManager_->Draw();

	for (auto suzunneModel : suzunnes) {
		suzunneModel->Draw();
	}
	terrain->Draw();

	editor.RailDarw();
	enemyManager_->Draw();

	ParticleManager::GetInstance()->Draw();

	Line3dDrawer::GetInstance()->Line3dDrawer::DrawLine3d({ -4.0f,3.0f,8.0f }, { 10.0f,5.0f,-1.0f }, { 1.0f,1.0f,0.0f,1.0f });
	editor.Draw();
	Line3dDrawer::GetInstance()->Render();

#pragma endregion


#pragma region 前景スプライト

	dxCommon_->PreSpriteDraw();
	scoreArea->Draw();

#pragma endregion

	dxCommon_->Command();
	dxCommon_->PostEffect();


}

bool GameScene::IsLineCollisionSphere(const Vector3& P1, const Vector3& P2, const Vector3& C, float r) {
	Vector3 lineVec = P2;
	Vector3 lineToCenter = C - P1;

	// 内積を使って最近接点を求める
	float t = (lineToCenter * lineVec) / (lineVec * lineVec);

	// tを0～1の範囲にクランプ（線分内に限定）
	t = std::clamp(t, 0.0f, 1.0f);

	// 線分上の最近接点を計算
	Vector3 closestPoint = P1 + lineVec * t;

	// 最近接点と球の中心との距離を計算
	float distanceToCenter = (closestPoint - C).Length();

	// 距離が半径以下なら衝突と判定
	return distanceToCenter <= r;
}

void GameScene::ApplyGlobalVariables() {
}
