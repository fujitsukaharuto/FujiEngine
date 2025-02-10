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

	sphere = std::make_unique<Object3d>();
	sphere->CreateSphere();

	suzunne = std::make_unique<Object3d>();
	suzunne->Create("suzanne.obj");

	float addDis = 1.0f;
	for (int i = 0; i < 3; i++) {

		std::unique_ptr<Object3d> newModel = std::make_unique<Object3d>();
		newModel->Create("suzanne.obj");
		newModel->transform.translate.x += addDis;
		newModel->transform.translate.z += addDis;
		newModel->transform.rotate.y = 3.14f;
		suzunnes.push_back(std::move(newModel));
		addDis += 0.5f;

	}

	fence = std::make_unique<Object3d>();
	fence->Create("Fence.obj");

	terrain = std::make_unique<Object3d>();
	terrain->Create("terrain.obj");

	mate = std::make_unique<Material>();
	mate->SetTextureNamePath("grass.png");
	mate->CreateMaterial();
	mate->SetColor({ 1.0f,0.0f,0.0f,1.0f });

	test = std::make_unique<Sprite>();
	test->Load("uvChecker.png");


	soundData1 = audio_->SoundLoadWave("xxx.wav");
	soundData2 = audio_->SoundLoadWave("mokugyo.wav");

	ApplyGlobalVariables();

	emit.count = 3;
	emit.frequencyTime = 20.0f;
	emit.name = "animetest";
	emit.pos = { 0.0f,2.0f,0.0f };
	emit.animeData.lifeTime = 40.0f;
	emit.RandomSpeed({ -0.1f,0.1f }, { -0.1f,0.1f }, { -0.1f,0.1f });
	emit.RandomTranslate({ -0.1f,0.1f }, { -0.1f,0.1f }, { -0.1f,0.1f });
}

void GameScene::Update() {

#ifdef _DEBUG

	ApplyGlobalVariables();

	ImGui::Begin("suzunne");

	ImGui::ColorEdit4("color", &color_.x);
	suzunne->SetColor(color_);
	ImGui::End();

	ImGui::Begin("Sphere");

	ImGui::DragFloat3("scale", &sphere->transform.scale.x, 0.01f);
	ImGui::DragFloat3("rotate", &sphere->transform.rotate.x, 0.01f);
	ImGui::DragFloat3("right", &rightDir.x,0.01f);
	rightDir = rightDir.Normalize();
	sphere->SetRightDir(rightDir);
	ImGui::End();

	emit.DebugGUI();

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

	if (input_->TriggerKey(DIK_5)) {
		emit.BurstAnime();
	}
	if (input_->TriggerKey(DIK_8)) {
		audio_->SoundPlayWave(soundData1);
	}
	if (input_->TriggerKey(DIK_9)) {
		audio_->SoundStopWave(soundData1);

	}
	if (input_->TriggerKey(DIK_7)) {
		audio_->SoundPlayWave(soundData2);
	}

	dxCommon_->UpDate();

	BlackFade();

	suzunne->transform.rotate.y = 3.14f;
	suzunne->transform.rotate.x += (0.05f) * FPSKeeper::DeltaTime();


	float rotaSpeed = 0.1f;
	for (auto& suzunneModel : suzunnes) {
		suzunneModel->transform.rotate.x += rotaSpeed * FPSKeeper::DeltaTime();
		//suzunneModel->transform.translate = Random::GetVector3({ -4.0f,4.0f }, { -4.0f,4.0f }, { -4.0f,4.0f });
		rotaSpeed += 0.05f;
	}


	sphere->transform.translate = spherevec;
	sphere->transform.rotate.y += 0.02f;

	fence->transform.translate = fencevec;
	fence->transform.rotate.x = 0.5f;

	
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
	for (auto& suzunneModel : suzunnes) {
		suzunneModel->Draw();
	}
	terrain->Draw();

	ParticleManager::GetInstance()->Draw();

#pragma endregion


#pragma region 前景スプライト

	dxCommon_->PreSpriteDraw();
	test->Draw();
	if (blackTime != 0.0f) {
		black_->Draw();
	}

#pragma endregion

}

void GameScene::BlackFade() {
	if (isChangeFase) {
		if (blackTime < blackLimmite) {
			blackTime += FPSKeeper::DeltaTime();
			if (blackTime >= blackLimmite) {
				blackTime = blackLimmite;
			}
		} else {
			SceneManager::GetInstance()->ChangeScene("RESULT", 40.0f);
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
	XINPUT_STATE pad;
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		if (blackTime == 0.0f) {
			isChangeFase = true;
		}
	} else if (Input::GetInstance()->GetGamepadState(pad)) {
		if (Input::GetInstance()->TriggerButton(PadInput::A)) {
			if (blackTime == 0.0f) {
				isChangeFase = true;
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
