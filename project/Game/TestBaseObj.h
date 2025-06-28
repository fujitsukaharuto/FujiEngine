#pragma once
#include "Game/OriginGameObject.h"
#include "ImGuiManager/ImGuiManager.h"
#include <numbers>
#include "Game/Collider/BaseCollider.h"
#include "Game/Collider/AABBCollider.h"
#include "Model/Line3dDrawer.h"

class TestBaseObj : public OriginGameObject {
public:
	TestBaseObj();
	~TestBaseObj()override;

	void Initialize()override;
	void Update()override;
	void Draw(Material* mate = nullptr, bool is = false)override;
	void DebugGUI()override;

	void OnCollisionEnter(const ColliderInfo& other);
	void OnCollisionStay(const ColliderInfo& other);
	void OnCollisionExit(const ColliderInfo& other);


	BaseCollider* GetCollider() { return collider_.get(); }

	std::string name_;

#ifdef _DEBUG
	void Debug()override;
	void DrawCollider();
#endif // _DEBUG

private:

	float time_;
	float omega_;
	Vector4 color_ = { 1.0f,1.0f,1.0f,1.0f };

	std::unique_ptr<Object3d> shadow_;
	std::unique_ptr<AABBCollider> collider_ = nullptr;
	bool isCollider_ = false;

};


inline TestBaseObj::TestBaseObj() {
}

inline TestBaseObj::~TestBaseObj() {
}

inline void TestBaseObj::Initialize() {
	OriginGameObject::Initialize();
	model_->Create("Sphere");
	model_->transform.translate = Vector3(0.0f, 4.0f, 5.0f);
	model_->transform.scale = Vector3(0.5f, 0.5f, 0.5f);

	shadow_ = std::make_unique<Object3d>();
	shadow_->Create("Sphere");
	shadow_->SetColor({ 0.0f,0.0f,0.0f,1.0f });
	shadow_->SetLightEnable(LightMode::kLightNone);
	shadow_->transform.translate = model_->transform.translate;
	shadow_->transform.translate.y = 0.15f;
	shadow_->transform.scale = { 0.5f,0.1f,0.5f };

	omega_ = 2.0f * std::numbers::pi_v<float> / 300.0f;
	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetCollisionStayCallback([this](const ColliderInfo& other) {OnCollisionStay(other); });
	collider_->SetCollisionExitCallback([this](const ColliderInfo& other) {OnCollisionExit(other); });
	collider_->SetTag("testBoss");
	collider_->SetWidth(0.5f);
	collider_->SetDepth(0.5f);
	collider_->SetHeight(0.5f);
}

inline void TestBaseObj::Update() {
	isCollider_ = false;
	
	//collider_->SetPos(model_->GetWorldPos());
	collider_->InfoUpdate();
}

inline void TestBaseObj::Draw([[maybe_unused]] Material* mate, bool is) {

	shadow_->Draw();

	OriginGameObject::Draw(mate, is);
}

inline void TestBaseObj::DebugGUI() {
#ifdef _DEBUG
	if (ImGui::CollapsingHeader(name_.c_str())) {
		OriginGameObject::DebugGUI();
		//collider_->SetPos(model_->GetWorldPos());

		collider_->DebugGUI();
	}
#endif // _DEBUG
}

inline void TestBaseObj::OnCollisionEnter([[maybe_unused]]const ColliderInfo& other) {

	isCollider_ = true;
	color_ = { 0.0f,0.0f,0.0f,1.0f };

}

inline void TestBaseObj::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {
	isCollider_ = true;
	color_ = { 0.0f,1.0f,1.0f,1.0f };
}

inline void TestBaseObj::OnCollisionExit([[maybe_unused]] const ColliderInfo& other) {
	color_ = { 1.0f,0.0f,1.0f,1.0f };
}

#ifdef _DEBUG
inline void TestBaseObj::Debug() {

	if (ImGui::CollapsingHeader(name_.c_str())) {
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
		bool isColl = collider_->GetIsCollisonCheck();
		ImGui::Checkbox("isCollision", &isColl);
		collider_->SetIsCollisonCheck(isColl);
	}
}
inline void TestBaseObj::DrawCollider() {

	std::array<Vector3, 8> v = collider_->GetWorldVertices();

	// 線を描画
	Line3dDrawer::GetInstance()->DrawLine3d(v[0], v[1], color_); // 底面の線
	Line3dDrawer::GetInstance()->DrawLine3d(v[1], v[2], color_);
	Line3dDrawer::GetInstance()->DrawLine3d(v[2], v[3], color_);
	Line3dDrawer::GetInstance()->DrawLine3d(v[3], v[0], color_);

	Line3dDrawer::GetInstance()->DrawLine3d(v[4], v[5], color_); // 上面の線
	Line3dDrawer::GetInstance()->DrawLine3d(v[5], v[6], color_);
	Line3dDrawer::GetInstance()->DrawLine3d(v[6], v[7], color_);
	Line3dDrawer::GetInstance()->DrawLine3d(v[7], v[4], color_);

	Line3dDrawer::GetInstance()->DrawLine3d(v[0], v[4], color_); // 側面の線
	Line3dDrawer::GetInstance()->DrawLine3d(v[1], v[5], color_);
	Line3dDrawer::GetInstance()->DrawLine3d(v[2], v[6], color_);
	Line3dDrawer::GetInstance()->DrawLine3d(v[3], v[7], color_);

}
#endif // _DEBUG
