#include "GameScene.h"
#include "ImGuiManager.h"
#include "ModelManager.h"
#include "GlobalVariables.h"

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
	delete pointLight;
	delete spotLight;
}

void GameScene::Initialize() {
	dxCommon_ = DXCom::GetInstance();
	input_ = Input::GetInstance();
	audio_ = Audio::GetInstance();

	GlobalVariables* globalvariables = GlobalVariables::GetInstance();
	const char* groupName = "Sphere";
	const char* groupName2 = "Fence";

	globalvariables->CreateGroup(groupName);
	globalvariables->AddItem(groupName, "parametar", spherePara);
	globalvariables->AddItem(groupName, "Position", spherevec);

	globalvariables->CreateGroup(groupName2);
	globalvariables->AddItem(groupName2, "parametar", fencePara);
	globalvariables->AddItem(groupName2, "Position", fencevec);


	pointLight = new PointLight();
	pointLight->Initialize();

	spotLight = new SpotLight();
	spotLight->Initialize();


	sphere = new Object3d();
	sphere->CreateSphere();
	sphere->SetPointLight(pointLight);
	sphere->SetSpotLight(spotLight);

	suzunne = new Object3d();
	suzunne->Create("suzanne.obj");
	suzunne->SetPointLight(pointLight);
	suzunne->SetSpotLight(spotLight);

	float addDis = 1.0f;
	for (int i = 0; i < 3; i++) {

		Object3d* newModel = new Object3d();
		newModel->Create("suzanne.obj");
		newModel->SetPointLight(pointLight);
		newModel->SetSpotLight(spotLight);
		newModel->transform.translate.x += addDis;
		newModel->transform.translate.z += addDis;
		newModel->transform.rotate.y = 3.14f;
		suzunnes.push_back(newModel);
		addDis += 0.5f;

	}

	fence = new Object3d();
	fence->Create("Fence.obj");
	fence->SetPointLight(pointLight);
	fence->SetSpotLight(spotLight);

	terrain = new Object3d();
	terrain->Create("terrain.obj");
	terrain->SetPointLight(pointLight);
	terrain->SetSpotLight(spotLight);

	test = new Sprite();
	test->Load("uvChecker.png");
	test->SetPointLight(pointLight);
	test->SetSpotLight(spotLight);

	soundData1 = audio_->SoundLoadWave("resource/xxx.wav");
	soundData2 = audio_->SoundLoadWave("resource/mokugyo.wav");

	ApplyGlobalVariables();

}

void GameScene::Update() {


#ifdef _DEBUG
	if (input_->TriggerKey(DIK_F12)) {
		if (isDebugCameraMode_) {
			isDebugCameraMode_ = false;
			dxCommon_->SetIsDebugCamera(false);
		} else {
			isDebugCameraMode_ = true;
			dxCommon_->SetIsDebugCamera(true);
		}
	}

	if (isDebugCameraMode_)
	{
		DebugCamera::GetInstance()->Update();
	}

	ApplyGlobalVariables();

	ImGui::Begin("suzunne");

	ImGui::ColorEdit4("color", &color_.X);
	suzunne->SetColor(color_);
	ImGui::End();

	ImGui::Begin("Sphere");

	ImGui::DragFloat3("scale", &sphere->transform.scale.x, 0.01f);
	ImGui::DragFloat3("rotate", &sphere->transform.rotate.x, 0.01f);
	ImGui::DragFloat3("right", &rightDir.x,0.01f);
	rightDir = rightDir.Normalize();
	sphere->SetRightDir(rightDir);
	ImGui::End();

	ImGui::Begin("pointlight");

	ImGui::DragFloat3("pos", &pointLight->pointLightData_->position.x, 0.01f);
	ImGui::DragFloat3("color", &pointLight->pointLightData_->color.X, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("intens", &pointLight->pointLightData_->intensity, 0.01f);
	ImGui::DragFloat("radius", &pointLight->pointLightData_->radius, 0.01f);
	ImGui::DragFloat("decay", &pointLight->pointLightData_->decay, 0.01f);

	ImGui::End();


	ImGui::Begin("spotlight");

	ImGui::DragFloat3("pos", &spotLight->spotLightData_->position.x, 0.01f);
	ImGui::DragFloat3("color", &spotLight->spotLightData_->color.X, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("intens", &spotLight->spotLightData_->intensity, 0.01f);
	ImGui::DragFloat3("direction", &spotLight->spotLightData_->direction.x, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("distance", &spotLight->spotLightData_->distance, 0.01f);
	ImGui::DragFloat("decay", &spotLight->spotLightData_->decay, 0.01f);
	ImGui::DragFloat("cosAngle", &spotLight->spotLightData_->cosAngle, 0.01f, -4.0f, 8.0f);
	ImGui::DragFloat("cosStart", &spotLight->spotLightData_->cosFalloffStart, 0.01f);
	if (spotLight->spotLightData_->cosFalloffStart <= spotLight->spotLightData_->cosAngle) {
		spotLight->spotLightData_->cosFalloffStart = (spotLight->spotLightData_->cosAngle) + 0.01f;
	}
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
	}
	if (input_->TriggerKey(DIK_9)) {
		audio_->SoundStopWave(soundData1);
	}
	if (input_->TriggerKey(DIK_7)) {
		audio_->SoundPlayWave(soundData2);
	}

	dxCommon_->UpDate();
	suzunne->transform.rotate.y = 3.14f;
	suzunne->transform.rotate.x += 0.05f;


	float rotaSpeed = 0.1f;
	for (auto suzunneModel : suzunnes) {
		suzunneModel->transform.rotate.x += rotaSpeed;

		rotaSpeed += 0.05f;
	}


	sphere->transform.translate = spherevec;
	sphere->transform.rotate.y += 0.02f;

	fence->transform.translate = fencevec;
	fence->transform.rotate.x = 0.5f;


}

void GameScene::Draw() {

#pragma region 背景描画


	dxCommon_->ClearDepthBuffer();
#pragma endregion


#pragma region 3Dオブジェクト
	dxCommon_->PreModelDraw();
	sphere->Draw();
	suzunne->Draw();
	fence->Draw();
	for (auto suzunneModel : suzunnes) {
		suzunneModel->Draw();
	}
	terrain->Draw();

#pragma endregion


#pragma region 前景スプライト

	dxCommon_->PreSpriteDraw();
	test->Draw();

#pragma endregion

	dxCommon_->Command();
	dxCommon_->PostEffect();


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
