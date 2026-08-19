#include "BossCore.h"
#include "Engine/Core/Debug/ImGuiManager.h"

#include "Game/GameObj/Enemy/Boss.h"

using namespace Graphics;
using namespace Collision;


BossCore::BossCore(Boss* pboss) {
	pBoss_ = pboss;
}

void BossCore::Initialize() {
	GameObject::GameObject::Initialize();
	GameObject::GameObject::CreateModel("Sphere");

	model_->LoadTransformFromJson("bossCore_transform.json");

	collider_ = AddCollider("testBoss");
	collider_->SetParent(&model_->GetTransform());
	collider_->SetOffset({ 0.0f,2.0f, -1.5f });
	collider_->SetWidth(5.0f);
	collider_->SetHeight(5.0f);
	collider_->SetDepth(5.0f);

	InitParameter();

	model_->SetParent(&pBoss_->GetTrans());
	model_->SetNoneScaleParent(true);

}

void BossCore::Update() {
	collider_->InfoUpdate();
}

void BossCore::Draw([[maybe_unused]] bool is) {
	// model_ は当たり判定用のアンカーなので描画しない
	DrawColliders();
}

void BossCore::DebugGUI() {
#ifdef _DEBUGMODE
	if (ImGui::CollapsingHeader("BossCore")) {
		model_->DebugGUI();
		collider_->DebugGUI();
	}
#endif // _DEBUG
}

void BossCore::ParameterGUI() {
#ifdef _DEBUGMODE

#endif // _DEBUG
}

void BossCore::InitParameter() {
}

void BossCore::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {
	if (other.tag == "playerBullet_strong") {
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
