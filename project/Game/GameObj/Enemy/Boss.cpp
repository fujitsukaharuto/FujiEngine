#include "Boss.h"

#include "Game/GameObj/Enemy/Behavior/BossRoot.h"
#include "Game/GameObj/Enemy/Behavior/BossAttack.h"

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


	ChangeBehavior(std::make_unique<BossRoot>(this));
}

void Boss::Update() {
	behavior_->Update();

	core_->Update();

	shadow_->transform.translate = model_->transform.translate;
	shadow_->transform.translate.y = 0.15f;
	collider_->SetPos(model_->GetWorldPos());
	collider_->InfoUpdate();
}

void Boss::Draw(Material* mate) {
	shadow_->Draw();
	collider_->DrawCollider();

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
	attackCooldown_ = 300.0f;
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
