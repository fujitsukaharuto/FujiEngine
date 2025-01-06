#include "Enemy.h"

Enemy::Enemy() {
}

Enemy::~Enemy() {
}

void Enemy::Initialize() {
	OriginGameObject::Initialize();
	model_->Create("suzanne.obj");
	omega_ = 2.0f * std::numbers::pi_v<float> / 300.0f;
	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetCollisionStayCallback([this](const ColliderInfo& other) {OnCollisionStay(other); });
	collider_->SetCollisionExitCallback([this](const ColliderInfo& other) {OnCollisionExit(other); });
	collider_->SetTag("enemy");

	hitParticle1_.name = "hitParticle1";
	hitParticle2_.name = "hitParticle2";
	hitParticle3_.name = "hitParticle3";
	hitParticle4_.name = "hitParticle4";

	hitParticle1_.Load("hitParticle1");
	hitParticle2_.Load("hitParticle2");
	hitParticle3_.Load("hitParticle3");
	hitParticle4_.Load("hitParticle4");
}

void Enemy::Update() {
	isCollider_ = false;

	collider_->SetPos(model_->GetWorldPos());

	hitParticle1_.pos = model_->GetWorldPos();
	hitParticle2_.pos = model_->GetWorldPos();
	hitParticle3_.pos = model_->GetWorldPos();
	hitParticle4_.pos = model_->GetWorldPos();

#ifdef _DEBUG
	/*hitParticle1_.DebugGUI();
	hitParticle2_.DebugGUI();
	hitParticle3_.DebugGUI();
	hitParticle4_.DebugGUI();*/
#endif // _DEBUG
}

void Enemy::Draw(Material* mate) {
	OriginGameObject::Draw(mate);
}

void Enemy::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {
	if (other.tag == "attack") {
		isCollider_ = true;
		color_ = { 1.0f,0.0f,0.0f,1.0f };
		hitParticle1_.Burst();
		hitParticle2_.Burst();
		hitParticle3_.Burst();
		hitParticle4_.Burst();
	}
}

void Enemy::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {
	isCollider_ = true;
	color_ = { 0.0f,1.0f,1.0f,1.0f };
}

void Enemy::OnCollisionExit([[maybe_unused]] const ColliderInfo& other) {
	color_ = { 1.0f,0.0f,1.0f,1.0f };
}

#ifdef _DEBUG
void Enemy::Debug() {

	/*ImGui::Begin(name_.c_str());

	ImGui::DragFloat3("position", &model_->transform.translate.x, 0.1f);
	collider_->SetPos(model_->GetWorldPos());

	float w = collider_->GetWidth();
	float h = collider_->GetHeight();
	float d = collider_->GetDepth();
	ImGui::DragFloat("width", &w, 0.1f);
	ImGui::DragFloat("height", &h, 0.1f);
	ImGui::DragFloat("depth", &d, 0.1f);
	collider_->SetWidth(w);
	collider_->SetHeight(h);
	collider_->SetDepth(d);

	ImGui::End();*/

}

void Enemy::DrawCollider() {
	// 半サイズを計算
	float halfWidth = collider_->GetWidth() / 2.0f;
	float halfHeight = collider_->GetHeight() / 2.0f;
	float halfDepth = collider_->GetDepth() / 2.0f;

	// 8つの頂点を計算
	Vector3 v1 = { collider_->GetPos().x - halfWidth, collider_->GetPos().y - halfHeight, collider_->GetPos().z - halfDepth };
	Vector3 v2 = { collider_->GetPos().x + halfWidth, collider_->GetPos().y - halfHeight, collider_->GetPos().z - halfDepth };
	Vector3 v3 = { collider_->GetPos().x + halfWidth, collider_->GetPos().y + halfHeight, collider_->GetPos().z - halfDepth };
	Vector3 v4 = { collider_->GetPos().x - halfWidth, collider_->GetPos().y + halfHeight, collider_->GetPos().z - halfDepth };
	Vector3 v5 = { collider_->GetPos().x - halfWidth, collider_->GetPos().y - halfHeight, collider_->GetPos().z + halfDepth };
	Vector3 v6 = { collider_->GetPos().x + halfWidth, collider_->GetPos().y - halfHeight, collider_->GetPos().z + halfDepth };
	Vector3 v7 = { collider_->GetPos().x + halfWidth, collider_->GetPos().y + halfHeight, collider_->GetPos().z + halfDepth };
	Vector3 v8 = { collider_->GetPos().x - halfWidth, collider_->GetPos().y + halfHeight, collider_->GetPos().z + halfDepth };

	// 線を描画
	Line3dDrawer::GetInstance()->DrawLine3d(v1, v2, color_); // 底面の線
	Line3dDrawer::GetInstance()->DrawLine3d(v2, v3, color_);
	Line3dDrawer::GetInstance()->DrawLine3d(v3, v4, color_);
	Line3dDrawer::GetInstance()->DrawLine3d(v4, v1, color_);

	Line3dDrawer::GetInstance()->DrawLine3d(v5, v6, color_); // 上面の線
	Line3dDrawer::GetInstance()->DrawLine3d(v6, v7, color_);
	Line3dDrawer::GetInstance()->DrawLine3d(v7, v8, color_);
	Line3dDrawer::GetInstance()->DrawLine3d(v8, v5, color_);

	Line3dDrawer::GetInstance()->DrawLine3d(v1, v5, color_); // 側面の線
	Line3dDrawer::GetInstance()->DrawLine3d(v2, v6, color_);
	Line3dDrawer::GetInstance()->DrawLine3d(v3, v7, color_);
	Line3dDrawer::GetInstance()->DrawLine3d(v4, v8, color_);
}
#endif // _DEBUG


