#include "GameScene.h"
#include "ImGuiManager.h"
#include "GlobalVariables.h"
#include "Line3dDrawer.h"
#include "FPSKeeper.h"

#include "ParticleManager.h"



GameScene::GameScene() {}

GameScene::~GameScene() {
	delete sphere;
	delete suzunne;
	delete fence;
	for (auto suzunneModel : suzunnes) {
		delete suzunneModel;
	}
	delete terrain;
	delete test;
	delete player_;
	delete enemyManager_;

}

void GameScene::Initialize() {
	Init();

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

	sphere = new Object3d();
	sphere->CreateSphere();

	suzunne = new Object3d();
	suzunne->Create("suzanne.obj");

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

	fence = new Object3d();
	fence->Create("Fence.obj");

	terrain = new Object3d();
	terrain->Create("terrain.obj");


	test = new Sprite();
	test->Load("uvChecker.png");

	soundData1 = audio_->SoundLoadWave("resource/xxx.wav");
	soundData2 = audio_->SoundLoadWave("resource/mokugyo.wav");

	ApplyGlobalVariables();

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

	ApplyGlobalVariables();

	editor.Update();
	enemyManager_->Update();

#ifdef _DEBUG


	ImGui::Begin("fps");
	float fps = 1.0f / FPSKeeper::BaseTime();
	ImGui::Text("%f", fps);
	ImGui::End();

	ImGui::Begin("suzunne");

	ImGui::ColorEdit4("color", &color_.X);
	suzunne->SetColor(color_);
	ImGui::DragFloat3("scale", &suzunne->transform.scale.x, 0.01f);
	ImGui::End();

	ImGui::Begin("Sphere");

	ImGui::DragFloat3("scale", &sphere->transform.scale.x, 0.01f);
	ImGui::DragFloat3("rotate", &sphere->transform.rotate.x, 0.01f);
	ImGui::DragFloat3("right", &rightDir.x,0.01f);
	rightDir = rightDir.Normalize();
	sphere->SetRightDir(rightDir);
	ImGui::End();


#endif // _DEBUG

	if (input_->PushKey(DIK_LEFT)) {
		suzunne->transform.translate.x -= 0.05f;
	}
	if (input_->PushKey(DIK_RIGHT)) {
		suzunne->transform.translate.x += 0.05f;
	}
	if (input_->PushKey(DIK_UP)) {
		suzunne->transform.translate.y += 0.05f;
	}
	if (input_->PushKey(DIK_DOWN)) {
		suzunne->transform.translate.y -= 0.05f;
	}


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
	suzunne->transform.rotate.y = 3.14f;
	suzunne->transform.rotate.x += (0.05f) * FPSKeeper::DeltaTime();


	float rotaSpeed = 0.1f;
	for (auto suzunneModel : suzunnes) {
		suzunneModel->transform.rotate.x += rotaSpeed * FPSKeeper::DeltaTime();
		//suzunneModel->transform.translate = Random::GetVector3({ -4.0f,4.0f }, { -4.0f,4.0f }, { -4.0f,4.0f });
		rotaSpeed += 0.05f;
	}


	sphere->transform.translate = spherevec;
	sphere->transform.rotate.y += 0.02f;

	fence->transform.translate = fencevec;
	fence->transform.rotate.x = 0.5f;


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
	sphere->Draw();
	suzunne->Draw();
	fence->Draw();

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
	test->Draw();

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
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	const char* groupName = "Sphere";
	const char* groupName2 = "Fence";

	spherePara = globalVariables->GetFloatValue(groupName, "parametar");
	spherevec = globalVariables->GetVector3Value(groupName, "Position");

	fencePara = globalVariables->GetFloatValue(groupName2, "parametar");
	fencevec = globalVariables->GetVector3Value(groupName2, "Position");

}
