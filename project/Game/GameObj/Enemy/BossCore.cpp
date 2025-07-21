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
	collider_->SetParent(&model_->transform);
	collider_->SetTag("testBoss");
	collider_->SetOffset({ 0.0f,5.0f, -1.5f });
	collider_->SetWidth(5.0f);
	collider_->SetHeight(5.0f);
	collider_->SetDepth(5.0f);

	InitParameter();

	model_->SetParent(&pBoss_->GetAnimModel()->transform);
	model_->SetNoneScaleParent(true);

}

void BossCore::Update() {
	collider_->InfoUpdate();
}

void BossCore::Draw([[maybe_unused]] Material* mate, [[maybe_unused]] bool is) {
#ifdef _DEBUG
	collider_->DrawCollider();
#endif // _DEBUG
	//OriginGameObject::Draw(mate, is);
}

void BossCore::DebugGUI() {
#ifdef _DEBUG
	if (ImGui::CollapsingHeader("BossCore")) {
		model_->DebugGUI();
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
	if (other.tag == "playerBullet_strng") {
		pBoss_->ReduceBossHP(true);
	}
	if (other.tag == "playerBullet") {
		pBoss_->ReduceBossHP(false);
	}
}

void BossCore::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {
}

void BossCore::OnCollisionExit([[maybe_unused]] const ColliderInfo& other) {
}
