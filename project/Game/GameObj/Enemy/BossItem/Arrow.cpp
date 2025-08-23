#include "Arrow.h"
#include "Engine/Particle/ParticleManager.h"
#include <algorithm>

Arrow::Arrow() {
}

void Arrow::Initialize() {
	OriginGameObject::Initialize();
	OriginGameObject::CreateModel("Boss_Arrow.obj");

	collider_ = std::make_unique<AABBCollider>();
	collider_->SetTag("enemyAttack");
	collider_->SetParent(&model_->transform);
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetCollisionStayCallback([this](const ColliderInfo& other) {OnCollisionStay(other); });
	collider_->SetCollisionExitCallback([this](const ColliderInfo& other) {OnCollisionExit(other); });
	collider_->SetWidth(2.0f);
	collider_->SetHeight(3.0f);
	collider_->SetDepth(2.0f);

	velocity_ = { 0.0f,0.0f,0.0f };
	model_->transform.scale = { 5.0f,5.0f,5.0f };

	ParticleManager::Load(spark1_, "WaveWallSpark");
	ParticleManager::Load(spark2_, "WaveWallSpark");

	spark1_.SetParent(&model_->transform);
	spark2_.SetParent(&model_->transform);

	spark1_.pos_.x = 0.4f;
	spark1_.pos_.z = 1.40f;
	spark2_.pos_.x = -0.4f;
	spark2_.pos_.z = 1.40f;

	spark2_.emitSizeMax_.x = 0.0f;
	spark2_.emitSizeMin_.x = -0.75f;
	spark2_.para_.speedx = { -0.075f,0.0f };

}

void Arrow::Update() {
	if (!isLive_) return;

	EmitTimeUpdate();
	AnimaTimeUpdate();
	ArrivalTimeUpdate();

	collider_->InfoUpdate();
}

void Arrow::Draw([[maybe_unused]] Material* mate, [[maybe_unused]] bool is) {
	OriginGameObject::Draw();
}

void Arrow::DrawCollider() {
#ifdef _DEBUG
	collider_->DrawCollider();
#endif // _DEBUG
}

void Arrow::DebugGUI() {
#ifdef _DEBUG
	if (ImGui::CollapsingHeader("Arrow")) {
		collider_->DebugGUI();
		ParameterGUI();
	}
#endif // _DEBUG
}

void Arrow::ParameterGUI() {
#ifdef _DEBUG
	ImGui::Indent();
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Selected;
	if (ImGui::TreeNodeEx("Paramater", flags)) {

		if (ImGui::Button("Emit Arrow")) {
			InitArrow({ 0.0f,0.0f,5.0f }, 90.0f);
			if (endP_.z == startP_.z) {
				startP_.z -= 0.001f;
			}
			midtermP_ = startP_ * 0.5f + endP_ * 0.5f + Vector3(0.0f, 9.0f, 0.0f);
		}

		ImGui::DragFloat("arrival", &arrivalTime_, 0.001f, 0.0f, 30.0f);
		ImGui::DragFloat("ControlHeight", &controlHeight_, 0.1f, 0.0f, 30.0f);
		ImGui::DragFloat3("endP", &endP_.x, 0.1f, -30.0f, 30.0f);

		ImGui::TreePop();
	}
	ImGui::Unindent();
#endif // _DEBUG
}

void Arrow::InitParameter() {
}

void Arrow::InitArrow(const Vector3& pos, float emitTime) {
	model_->transform.translate = pos;
	model_->transform.rotate = Vector3::GetZeroVec();
	model_->transform.rotate.x = -1.565f;

	animationTime_ = totalAnimationTime_;
	emitTime_ = emitTime;
	arrivalTime_ = totalArrivalTime_;

	startP_ = pos;

	isLive_ = true;

	ParticleManager::GetParticleCSEmitter(emitterNumber_).emitter->prevTranslate = model_->transform.translate;
	ParticleManager::GetParticleCSEmitter(emitterNumber_).emitter->translate = model_->transform.translate;
}

void Arrow::TargetSetting(const Vector3& target) {
	if (animationTime_ <= 5.0f && arrivalTime_ >= totalAnimationTime_) {
		endP_ = target;
		if (endP_.z == startP_.z) {
			startP_.z -= 0.001f;
		}
		endP_.y = 0.0f;
		midtermP_ = startP_ * 0.5f + endP_ * 0.5f + Vector3(0.0f, controlHeight_, 0.0f);
	}
}

void Arrow::EmitTimeUpdate() {
	if (emitTime_ > 0.0f) {
		emitTime_ -= FPSKeeper::DeltaTime();
	}
}

void Arrow::AnimaTimeUpdate() {
	if (emitTime_ > 0.0f) return;
	if (animationTime_ > 0.0f) {
		animationTime_ -= FPSKeeper::DeltaTime();
		if (animationTime_ < 0.0f)
			animationTime_ = 0.0f;

		float t = 1.0f - (animationTime_ / totalAnimationTime_); // 0->1 の割合
		float rotationAmount = t * 4.0f * std::numbers::pi_v<float>; // 2回転
		model_->transform.rotate.y = rotationAmount;
		if (animationTime_ == 0.0f)
			model_->transform.rotate.y = 0.0f;
		if (animationTime_ <= 0.0f) {
			ParticleManager::GetParticleCSEmitter(emitterNumber_).isEmit = true;
		}
	}
}

void Arrow::ArrivalTimeUpdate() {
	if (emitTime_ > 0.0f || animationTime_ > 0.0f) return;
	if (arrivalTime_ > 0.0f) {

		ParticleManager::GetParticleCSEmitter(emitterNumber_).emitter->prevTranslate = model_->transform.translate;

		float pret = (std::min)((1.0f - arrivalTime_ / totalArrivalTime_), 1.0f);
		arrivalTime_ -= FPSKeeper::DeltaTime();
		float t = (std::min)((1.0f - arrivalTime_ / totalArrivalTime_), 1.0f);
		Vector3 pos = (1.0f - t) * (1.0f - t) * startP_ + 2.0f * (1.0f - t) * t * midtermP_ + t * t * endP_;
		model_->transform.translate = pos;
		ParticleManager::GetParticleCSEmitter(emitterNumber_).emitter->translate = model_->transform.translate;

		Vector3 dir = (2.0f * (1.0f - t)) * (midtermP_ - startP_) + (2.0f * t) * (endP_ - midtermP_);
		dir = dir.Normalize();
		Vector3 predir = (2.0f * (1.0f - pret)) * (midtermP_ - startP_) + (2.0f * pret) * (endP_ - midtermP_);
		predir = predir.Normalize();

		Quaternion rot = Quaternion::LookRotation(dir);
		Quaternion prerot = Quaternion::LookRotation(predir);
		Quaternion newRot = Quaternion::Slerp(prerot, rot, 0.1f);

		model_->transform.rotate = Quaternion::QuaternionToEuler(newRot);
	} else {
		isLive_ = false;
		ParticleManager::GetParticleCSEmitter(emitterNumber_).isEmit = false;
	}
}

void Arrow::GPUEmitterSetting() {
	ParticleManager::GetParticleCSEmitter(emitterNumber_).emitter->count = 300;
	ParticleManager::GetParticleCSEmitter(emitterNumber_).emitter->lifeTime = 30.0f;
	ParticleManager::GetParticleCSEmitter(emitterNumber_).emitter->radius = 0.0f;
	ParticleManager::GetParticleCSEmitter(emitterNumber_).emitter->scale = { 1.0f,1.0f,1.0f };
	ParticleManager::GetParticleCSEmitter(emitterNumber_).emitter->colorMax = { 1.0f,0.0f,0.0f };
	ParticleManager::GetParticleCSEmitter(emitterNumber_).emitter->colorMin = { 1.0f,0.0f,0.0f };
}

void Arrow::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {
}

void Arrow::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {
}

void Arrow::OnCollisionExit([[maybe_unused]] const ColliderInfo& other) {
}

void Arrow::SetIsLive(bool is) {
	isLive_ = is;
	ParticleManager::GetParticleCSEmitter(emitterNumber_).isEmit = false;
}
