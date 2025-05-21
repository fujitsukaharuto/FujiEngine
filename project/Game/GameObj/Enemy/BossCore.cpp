#include "BossCore.h"

#include "Game/GameObj/Enemy/Boss.h"

BossCore::BossCore(Boss* pboss) {
	pBoss_ = pboss;
}

void BossCore::Initialize() {
	OriginGameObject::Initialize();
	OriginGameObject::CreateModel("Sphere");

	model_->LoadTransformFromJson("bossCore_transform.json");

	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetCollisionStayCallback([this](const ColliderInfo& other) {OnCollisionStay(other); });
	collider_->SetCollisionExitCallback([this](const ColliderInfo& other) {OnCollisionExit(other); });
	collider_->SetTag("testBoss");
	collider_->SetWidth(0.5f);
	collider_->SetHeight(0.5f);
	collider_->SetDepth(0.5f);

	InitParameter();

	model_->SetParent(pBoss_->GetModel());

}

void BossCore::Update() {
	collider_->SetPos(model_->GetWorldPos());
	collider_->InfoUpdate();
}

void BossCore::Draw(Material* mate) {
	collider_->DrawCollider();
	OriginGameObject::Draw(mate);
}

void BossCore::DebugGUI() {
#ifdef _DEBUG
	if (ImGui::CollapsingHeader("BossCore")) {
		model_->DebugGUI();
		collider_->SetPos(model_->GetWorldPos());

		collider_->DebugGUI();

	}
#endif // _DEBUG
}

void BossCore::ParameterGUI() {
#ifdef _DEBUG

#endif // _DEBUG
}

void BossCore::InitParameter() {
}

void BossCore::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {
}

void BossCore::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {
}

void BossCore::OnCollisionExit([[maybe_unused]] const ColliderInfo& other) {
}
