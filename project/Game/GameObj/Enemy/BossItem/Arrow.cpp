#include "Arrow.h"
#include "Engine/Particle/ParticleManager.h"
#include "Engine/Model/Object3d.h"
#include "Engine/Math/Random/Random.h"
#include <algorithm>

using namespace Audio;
using namespace Core;
using namespace Graphics;
using namespace Math;


Arrow::Arrow() {
}

Arrow::~Arrow() {
	ParticleManager::GetSphereEmitter(hitEmitIndex_).SetEmit(false);
	ParticleManager::GetSphereEmitter(hitEmitTrailIndex_).SetEmit(false);
	if (isArrow_) {
		ParticleManager::GetSphereEmitter(emitterNumber_).SetEmit(false);
	}
}

void Arrow::Initialize() {
	OriginGameObject::Initialize();
	OriginGameObject::CreateModel("Boss_Arrow.obj");

	collider_ = std::make_unique<AABBCollider>();
	collider_->SetTag("enemyAttack");
	collider_->SetParent(&model_->GetTransform());
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetCollisionStayCallback([this](const ColliderInfo& other) {OnCollisionStay(other); });
	collider_->SetCollisionExitCallback([this](const ColliderInfo& other) {OnCollisionExit(other); });
	collider_->SetWidth(2.0f);
	collider_->SetHeight(3.0f);
	collider_->SetDepth(2.0f);

	arrivalWarningPotion_ = std::make_unique<Object3d>();
	arrivalWarningPotion_->Create("Sphere");
	arrivalWarningPotion_->SetTexture("white2x2.png");
	arrivalWarningPotion_->SetColor({ 0.8f,0.0f,0.0f,0.4f });
	arrivalWarningPotion_->SetLightEnable(LightMode::kLightNone);
	arrivalWarningPotion_->GetTransform().scale = { 2.0f,0.0f,2.0f };
	arrivalWarningPotion_->GetTransform().scale.y = 0.1f;

	velocity_ = { 0.0f,0.0f,0.0f };
	model_->GetTransform().scale = { 5.0f,5.0f,5.0f };

	ParticleManager::Load(spark1_, "lightning");
	ParticleManager::Load(spark2_, "lightningSphere");
	ParticleManager::Load(spark3_, "lightningParticle");

	ParticleManager::Load(hit_, "arrowHit");
	ParticleManager::Load(hitParticle_, "arrowParticle");
	ParticleManager::Load(hitExpand_, "arrowExpandParticle");
	hitEmitIndex_ = ParticleManager::GetInstance()->InitGPUEmitter();
	hitEmitTrailIndex_ = ParticleManager::GetInstance()->InitGPUEmitter();
	ParticleManager::GetSphereEmitter(hitEmitIndex_).Load("arrowHitCS");
	ParticleManager::GetSphereEmitter(hitEmitTrailIndex_).Load("arrowHitTrailCS");

	spark1_.frequencyTime_ = 0.0f;
	spark3_.frequencyTime_ = 0.0f;

	throwSE_ = &AudioPlayer::GetInstance()->SoundLoadWave("arrowThrowSE.wav");
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
	if (isArrow_ && arrivalTime_ > 0.0f && animationTime_ < totalAnimationTime_) {
		arrivalWarningPotion_->Draw();
	}
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
	if (ImGui::TreeNodeEx("Parameter", flags)) {

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
	model_->GetTransform().translate = pos;
	model_->GetTransform().rotate = Vector3::GetZeroVec();
	model_->GetTransform().rotate.x = -std::numbers::pi_v<float> * 0.5f;

	animationTime_ = totalAnimationTime_;
	emitTime_ = emitTime;
	arrivalTime_ = totalArrivalTime_;

	startP_ = pos;

	isLive_ = true;

	auto& emitter = ParticleManager::GetSphereEmitter(emitterNumber_);
	emitter.data_.prevTranslate = model_->GetTransform().translate;
	emitter.data_.translate = model_->GetTransform().translate;
}

void Arrow::TargetSetting(const Vector3& target) {
	if (animationTime_ <= totalAnimationTime_ && arrivalTime_ >= totalArrivalTime_) {
		endP_ = target;
		if (endP_.z == startP_.z) { // ｚ軸が同じになり回転がおかしくならないようにする
			startP_.z -= 0.001f;
		}
		endP_.y = 0.0f;
		arrivalWarningPotion_->GetTransform().translate = endP_;
		arrivalWarningPotion_->GetTransform().translate.y += 0.1f;
		midtermP_ = startP_ * 0.5f + endP_ * 0.5f + Vector3(0.0f, controlHeight_, 0.0f);
	}
}

void Arrow::EmitTimeUpdate() {
	if (emitTime_ > 0.0f) {
		emitTime_ -= FPSKeeper::DeltaTimeFrame();
	}
}

void Arrow::AnimaTimeUpdate() {
	if (emitTime_ > 0.0f) return;
	if (animationTime_ > 0.0f) { // 発射前の回転を行う
		animationTime_ -= FPSKeeper::DeltaTimeFrame();
		if (animationTime_ < 0.0f)
			animationTime_ = 0.0f;

		float t = 1.0f - (animationTime_ / totalAnimationTime_); // 0->1 の割合
		float rotationAmount = t * 4.0f * std::numbers::pi_v<float>; // 2回転
		model_->GetTransform().rotate.y = rotationAmount;
		if (animationTime_ == 0.0f)
			model_->GetTransform().rotate.y = 0.0f;
		if (animationTime_ <= 0.0f) {
			ParticleManager::GetSphereEmitter(emitterNumber_).SetEmit(true);
			AudioPlayer::GetInstance()->SoundPlayWave(*throwSE_, 0.2f);
		}
	}
}

void Arrow::ArrivalTimeUpdate() {
	if (emitTime_ > 0.0f || animationTime_ > 0.0f) return;
	if (arrivalTime_ > 0.0f) {
		auto& emitter = ParticleManager::GetSphereEmitter(emitterNumber_);
		emitter.data_.prevTranslate = model_->GetTransform().translate;
		
		float pret = (std::min)((1.0f - arrivalTime_ / totalArrivalTime_), 1.0f);
		arrivalTime_ -= FPSKeeper::DeltaTimeFrame();
		// 放物線の挙動を制御する
		float t = (std::min)((1.0f - arrivalTime_ / totalArrivalTime_), 1.0f);
		Vector3 pos = (1.0f - t) * (1.0f - t) * startP_ + 2.0f * (1.0f - t) * t * midtermP_ + t * t * endP_;
		model_->GetTransform().translate = pos;
		emitter.data_.translate = model_->GetTransform().translate;

		// 回転を決める
		Vector3 dir = (2.0f * (1.0f - t)) * (midtermP_ - startP_) + (2.0f * t) * (endP_ - midtermP_);
		dir = dir.Normalize();
		Vector3 predir = (2.0f * (1.0f - pret)) * (midtermP_ - startP_) + (2.0f * pret) * (endP_ - midtermP_);
		predir = predir.Normalize();

		Quaternion rot = Quaternion::LookRotation(dir);
		Quaternion prerot = Quaternion::LookRotation(predir);
		Quaternion newRot = Quaternion::SLerp(prerot, rot, 0.1f);

		model_->GetTransform().rotate = Quaternion::QuaternionToEuler(newRot);
	} else {
		isLive_ = false;
		hitParticle_.pos_ = model_->GetTransform().translate;
		hitExpand_.pos_ = model_->GetTransform().translate;
		hitParticle_.Emit();
		hitExpand_.Emit();
		hit_.pos_ = model_->GetTransform().translate;
		hit_.Emit();
		ParticleManager::GetSphereEmitter(hitEmitIndex_).SetPos(model_->GetTransform().translate);
		ParticleManager::GetSphereEmitter(hitEmitIndex_).Emit();
		ParticleManager::GetSphereEmitter(hitEmitTrailIndex_).SetPos(model_->GetTransform().translate);
		ParticleManager::GetSphereEmitter(hitEmitTrailIndex_).Emit();
		ParticleManager::GetSphereEmitter(emitterNumber_).SetEmit(false);
	}
}

void Arrow::GPUEmitterSetting() {
	auto& emitter = ParticleManager::GetSphereEmitter(emitterNumber_);
	emitter.SetCount(300);
	emitter.SetLifeTime(0.5f);
	emitter.SetRadius(0.0f);
	emitter.SetScale({ 1.0f,1.0f,1.0f });
	emitter.SetColorRandom({1.0f,0.0f,0.0f}, { 1.0f,0.0f,0.0f });
}

void Arrow::RodUpdate() {
	if (!isLive_) return;

	FlyTimeUpdate();
	FallTimeUpdate();
	BrokeTimeUpdate();

}

void Arrow::InitRod(const Vector3& pos, float time) {
	model_->GetTransform().translate = pos;
	model_->GetTransform().rotate = Vector3::GetZeroVec();
	model_->GetTransform().rotate.x = std::numbers::pi_v<float> * 0.5f;;

	flyTime_ = time;
	fallTime_ = maxFallTime_;
	brokeTime_ = maxBrokeTime_;

	isLive_ = true;
	isLightNing_ = true;
}

void Arrow::FlyTimeUpdate() {
	if (flyTime_ > 0.0f) {
		flyTime_ -= FPSKeeper::DeltaTimeFrame();
	}
}

void Arrow::FallTimeUpdate() {
	if (flyTime_ > 0.0f) return;
	if (fallTime_ > 0.0f) {
		fallTime_ -= FPSKeeper::DeltaTimeFrame();
		if (fallTime_ < 0.0f)
			fallTime_ = 0.0f;

		float t = 1.0f - (fallTime_ / maxFallTime_);
		float fallPos = std::lerp(9.0f, 1.0f, t);
		model_->GetTransform().translate.y = fallPos;
		if (fallTime_ == 0.0f) {
			hitParticle_.pos_ = model_->GetTransform().translate;
			hitParticle_.pos_.y -= 1.0f;
			hitParticle_.Emit();
			hit_.pos_ = model_->GetTransform().translate;
			hit_.pos_.y -= 1.0f;
			hit_.Emit();
			ParticleManager::GetSphereEmitter(hitEmitIndex_).SetPos(model_->GetTransform().translate);
			ParticleManager::GetSphereEmitter(hitEmitIndex_).Emit();
		}
	}
}

void Arrow::BrokeTimeUpdate() {
	if (flyTime_ > 0.0f || fallTime_ > 0.0f) return;
	if (brokeTime_ > 0.0f) {
		brokeTime_ -= FPSKeeper::DeltaTimeFrame();
		if (brokeTime_ <= 0.0f) {
			isBroke_ = true;

			spark2_.pos_ = model_->GetTransform().translate;
			spark2_.pos_.y = 0.0f;
			spark3_.pos_ = model_->GetTransform().translate;
			spark3_.pos_.y = 0.0f;
			spark2_.Emit();
			spark3_.Emit();
		}
		if (brokeTime_ <= 5.0f) {
			if (isLightNing_) {
				spark1_.pos_ = model_->GetTransform().translate;
				spark1_.pos_.y += 40.0f;
				spark1_.particleRotate_.y = Random::GetFloat(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);

				spark1_.Emit();
				isLightNing_ = false;
			}
		}
	} else {
		isLive_ = false;
	}
}

bool Arrow::GetIsBroke() {
	bool result = isBroke_;
	if (isBroke_) {
		isBroke_ = false;
	}
	return result;
}

void Arrow::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {
}

void Arrow::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {
}

void Arrow::OnCollisionExit([[maybe_unused]] const ColliderInfo& other) {
}

void Arrow::SetIsLive(bool is) {
	isLive_ = is;
	if (isArrow_) {
		ParticleManager::GetSphereEmitter(emitterNumber_).SetEmit(false);
	}
}
