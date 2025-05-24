#include "Boss.h"
#include "Engine/Particle/ParticleManager.h"

#include "Game/GameObj/Enemy/Behavior/BossRoot.h"
#include "Game/GameObj/Enemy/Behavior/BossAttack.h"

#include "Game/GameObj/Player/Player.h"


Boss::Boss() {
}

void Boss::Initialize() {
	OriginGameObject::Initialize();
	OriginGameObject::CreateModel("cube.obj");

	model_->LoadTransformFromJson("boss_transform.json");

	shadow_ = std::make_unique<Object3d>();
	shadow_->Create("Sphere");
	shadow_->SetColor({ 0.0f,0.0f,0.0f,1.0f });
	shadow_->SetLightEnable(LightMode::kLightNone);
	shadow_->transform.translate = model_->transform.translate;
	shadow_->transform.translate.y = 0.15f;
	shadow_->transform.scale.y = 0.1f;

	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetCollisionStayCallback([this](const ColliderInfo& other) {OnCollisionStay(other); });
	collider_->SetCollisionExitCallback([this](const ColliderInfo& other) {OnCollisionExit(other); });
	collider_->SetTag("Boss");
	collider_->SetWidth(4.0f);
	collider_->SetHeight(8.0f);
	collider_->SetDepth(3.0f);

	InitParameter();

	core_ = std::make_unique<BossCore>(this);
	core_->Initialize();

	ParticleManager::Load(waveAttack1, "ShockRay");
	ParticleManager::Load(waveAttack2, "ShockWaveGround");
	ParticleManager::Load(waveAttack3, "ShockWaveParticle");
	ParticleManager::Load(waveAttack4, "ShockWave");

	waveAttack1.frequencyTime_ = 0.0f;
	waveAttack2.frequencyTime_ = 0.0f;
	waveAttack3.frequencyTime_ = 0.0f;
	waveAttack4.frequencyTime_ = 0.0f;

	waveAttack1.isAddRandomSize_ = true;
	waveAttack1.addRandomMax_ = { 0.75f,1.2f };
	waveAttack1.addRandomMin_.y = -0.5f;


	ChangeBehavior(std::make_unique<BossRoot>(this));
}

void Boss::Update() {
	behavior_->Update();

	core_->Update();

	UpdateWaveWall();

	shadow_->transform.translate = model_->transform.translate;
	shadow_->transform.translate.y = 0.15f;
	collider_->SetPos(model_->GetWorldPos());
	collider_->InfoUpdate();
}

void Boss::Draw(Material* mate) {
	shadow_->Draw();
	collider_->DrawCollider();

	for (auto& wall : walls_) {
		if (!wall->GetIsLive())continue;
		wall->Draw();
	}

	core_->Draw();
	OriginGameObject::Draw(mate);
}

void Boss::DebugGUI() {
#ifdef _DEBUG
	if (ImGui::CollapsingHeader("Boss")) {
		model_->DebugGUI();
		collider_->SetPos(model_->GetWorldPos());
		collider_->DebugGUI();
	}
	core_->DebugGUI();
#endif // _DEBUG
}

void Boss::ParameterGUI() {
#ifdef _DEBUG

#endif // _DEBUG
}

void Boss::InitParameter() {
	attackCooldown_ = 600.0f;

	for (int i = 0; i < 3; i++) {
		std::unique_ptr<WaveWall> wall;
		wall = std::make_unique<WaveWall>();
		wall->Initialize();
		walls_.push_back(std::move(wall));
	}

}

void Boss::UpdateWaveWall() {
	for (auto& wall : walls_) {
		if (!wall->GetIsLive())continue;
		wall->CalculetionFollowVec(pPlayer_->GetWorldPos());
		wall->Update();
	}
}

void Boss::WaveWallAttack() {
	int count = 0;

	Vector3 wavePos = { 0.0f,0.0f,4.5f };
	Matrix4x4 rotateMatrix = MakeRotateYMatrix(model_->transform.rotate.y);
	wavePos = TransformNormal(wavePos, rotateMatrix);
	wavePos += model_->transform.translate;
	wavePos.y = 0.0f;
	for (auto& wall : walls_) {
		if (count == 3) break;
		if (wall->GetIsLive()) continue;
		Vector3 velocity = { 0.0f,0.0f,1.0f };
		if (count == 1) velocity = Vector3(-1.0f, 0.0f, 1.0f).Normalize();
		if (count == 2) velocity = Vector3(1.0f, 0.0f, 1.0f).Normalize();

		wall->InitWave(wavePos, velocity);
		count++;
	}
	waveAttack1.pos_ = wavePos;
	waveAttack2.pos_ = wavePos;
	waveAttack3.pos_ = wavePos;
	waveAttack4.pos_ = wavePos;

	waveAttack1.Emit();
	waveAttack2.Emit();
	waveAttack3.Emit();
	waveAttack4.Emit();

}

///= Behavior =================================================================*/
void Boss::ChangeBehavior(std::unique_ptr<BaseBossBehavior> behavior) {
	behavior_ = std::move(behavior);
}

///= Collision ================================================================*/
void Boss::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {
}

void Boss::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {
}

void Boss::OnCollisionExit([[maybe_unused]] const ColliderInfo& other) {
}
