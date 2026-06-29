#include "Boss.h"
#include "Engine/Particle/ParticleManager.h"
#include "Engine/Math/Random/Random.h"
#include "Engine/Camera/CameraManager.h"
#include "Engine/Editor/JsonSerializer.h"
#include <numbers>

#include "Game/GameObj/Enemy/Behavior/BossRoot.h"
#include "Game/GameObj/Enemy/Behavior/BossAttack.h"
#include "Game/GameObj/Enemy/Behavior/BossJumpAttack.h"
#include "Game/GameObj/Enemy/Behavior/BossSwordAttack.h"
#include "Game/GameObj/Enemy/Behavior/BossBeamAttack.h"
#include "Game/GameObj/Enemy/Behavior/BossAreaAttack.h"
#include "Game/GameObj/Enemy/Behavior/BossArrowAttack.h"
#include "Game/GameObj/Enemy/Behavior/BossRodFall.h"
#include "Game/GameObj/Enemy/Behavior/BossDushAttack.h"

#include "Game/GameObj/Player/Player.h"

using namespace Audio;
using namespace Core;
using namespace Graphics;
using namespace Math;



void Boss::Initialize() {
	OriginGameObject::Initialize();
	OriginGameObject::CreateAnimeModel("T_boss.gltf");
	animeModel_->LoadAnimationFile("T_boss.gltf");

	animeModel_->LoadTransformFromJson("boss_transform.json");

	shadow_ = std::make_unique<Object3d>();
	shadow_->Create("Sphere");
	shadow_->SetTexture("white2x2.png");
	shadow_->SetColor({ 0.02f,0.02f,0.02f,0.0f });
	shadow_->SetLightEnable(LightMode::kLightNone);
	shadow_->GetTransform().translate = animeModel_->GetTransform().translate;
	shadow_->GetTransform().translate.y = kShadowPosY_;
	shadow_->GetTransform().scale = { 3.0f,0.0f,3.0f };
	shadow_->GetTransform().scale.y = 0.1f;

	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetCollisionStayCallback([this](const ColliderInfo& other) {OnCollisionStay(other); });
	collider_->SetCollisionExitCallback([this](const ColliderInfo& other) {OnCollisionExit(other); });
	collider_->SetParent(&animeModel_->GetTransform());
	collider_->SetTag("Boss");
	collider_->SetOffset({ 0.0f,7.0f,0.0f });
	collider_->SetWidth(7.5f);
	collider_->SetHeight(15.0f);
	collider_->SetDepth(7.0f);

	InitParameter();

	core_ = std::make_unique<BossCore>(this);
	core_->Initialize();

	beam_ = std::make_unique<Beam>();
	beam_->Initialize();
	beam_->SetBossParent(this);

	InitChargeParent();

	InitItemParent();

	InitEmitter();

	actionList_ = {
		"Root","Wave","Beam","Jump","Sword","Area","Arrow","FallRod","Dush",
	};
	LoadPhase();
	ChangeBehavior(std::make_unique<BossRoot>(this));
	InitSummon();
	animeModel_->ChangeAnimation("idle");

	jumpAttackSE_ = &AudioPlayer::GetInstance()->SoundLoadWave("jumpAttackSE.wav");
}

void Boss::InitParameter() {
	attackCooldown_ = kInitAttackCooldown_;
	bossHp_ = kMaxBossHp_;

	params_.beam.parentRotateStep = std::numbers::pi_v<float>*0.25f;
	itemManager_ = std::make_unique<BossItemManager>();
	itemManager_->Initialize(this);

	for (int i = 0; i < kHpSegmentCount_; i++) {
		std::unique_ptr<Sprite> hpTex;
		hpTex = std::make_unique<Sprite>();
		hpTex->Load("white2x2.png");
		hpTex->SetColor(damageColor1_);
		hpSprites_.push_back(std::move(hpTex));
		hpSprites_[i]->SetPos({ hpStartPos_.x + (hpSize_.x * i) + (hpIndent * i), hpStartPos_.y, 0.0f });
		hpSprites_[i]->SetSize(hpSize_);
	}

	for (int i = 0; i < kHpFrameCount_; i++) {
		std::unique_ptr<Sprite> hpTex;
		hpTex = std::make_unique<Sprite>();
		hpTex->Load("white2x2.png");
		hpTex->SetColor({ 0.2f,0.05f,0.6f,1.0f });
		hpFrame_.push_back(std::move(hpTex));
	}
	hpFrame_[0]->SetPos({ hpFrameStartPos_.x, hpFrameStartPos_.y, 0.0f });
	hpFrame_[0]->SetSize(hpFrameSize_);
	hpFrame_[1]->SetColor({ 0.1f,0.1f,0.1f,1.0f });
	hpFrame_[1]->SetPos({ hpFrameStartPos_.x, hpFrameStartPos_.y, 0.0f });
	hpFrame_[1]->SetSize(hpFrameInSize_);

}

void Boss::InitEmitter() {
	ParticleManager::Load(waveAttack1, "ShockRay");
	ParticleManager::Load(waveAttack2, "ShockWaveGround");
	ParticleManager::Load(waveAttack3, "ShockWaveParticle");
	ParticleManager::Load(waveAttack4, "ShockWave");
	ParticleManager::Load(jumpWave_, "JumpShockWave");
	waveCSEmitIndex_ = ParticleManager::GetInstance()->InitGPUEmitter();
	jumpCSEmitIndex_ = ParticleManager::GetInstance()->InitGPUEmitter();
	halfAuraCS_ = ParticleManager::GetInstance()->InitGPUEmitter();
	halfSmallAuraCS_ = ParticleManager::GetInstance()->InitGPUEmitter();
	leftHandAuraCS_ = ParticleManager::GetInstance()->InitGPUEmitter();
	rightHandAuraCS_ = ParticleManager::GetInstance()->InitGPUEmitter();
	ParticleManager::GetSphereEmitter(waveCSEmitIndex_).Load("shockWaveCS");
	ParticleManager::GetSphereEmitter(jumpCSEmitIndex_).Load("jumpCSEmit");
	ParticleManager::GetSphereEmitter(halfAuraCS_).Load("bossAura");
	ParticleManager::GetSphereEmitter(halfSmallAuraCS_).Load("bossSmallAura");
	ParticleManager::GetSphereEmitter(leftHandAuraCS_).Load("bossHandAura");
	ParticleManager::GetSphereEmitter(rightHandAuraCS_).Load("bossHandAura");

	waveAttack1.frequencyTime_ = 0.0f;
	waveAttack2.frequencyTime_ = 0.0f;
	waveAttack3.frequencyTime_ = 0.0f;
	waveAttack4.frequencyTime_ = 0.0f;
	waveAttack1.pos_.z = kWaveAttackPosZ_;
	waveAttack2.pos_.z = kWaveAttackPosZ_;
	waveAttack3.pos_.z = kWaveAttackPosZ_;
	waveAttack4.pos_.z = kWaveAttackPosZ_;
	jumpWave_.frequencyTime_ = 0.0f;

	waveAttack1.isAddRandomSize_ = true;
	waveAttack1.addRandomMax_ = { 0.75f,1.2f };
	waveAttack1.addRandomMin_.y = -0.5f;

	waveAttack1.SetParent(&waveParent_->GetTransform());
	waveAttack2.SetParent(&waveParent_->GetTransform());
	waveAttack3.SetParent(&waveParent_->GetTransform());
	waveAttack4.SetParent(&waveParent_->GetTransform());

	ParticleManager::Load(charge9_, "BeamCharge5");
	ParticleManager::Load(charge10_, "BeamCharge9");
	ParticleManager::Load(charge11_, "BeamCharge8");
	ParticleManager::Load(charge12_, "BeamCharge6");
	ParticleManager::Load(charge13_, "BeamCharge7");
	ParticleManager::Load(charge14_, "BeamCharge10");
	ParticleManager::Load(charge15_, "BeamCharge11");

	charge12_.frequencyTime_ = 0.0f;
	charge13_.frequencyTime_ = 0.0f;
	charge14_.frequencyTime_ = 0.0f;

	charge9_.SetParent(&chargeParents_[0]->GetTransform());
	charge10_.SetParent(&chargeParents_[0]->GetTransform());
	charge11_.SetParent(&chargeParents_[0]->GetTransform());
	charge12_.SetParent(&chargeParents_[0]->GetTransform());
	charge13_.SetParent(&chargeParents_[0]->GetTransform());
	charge14_.SetParent(&chargeParents_[0]->GetTransform());
	charge15_.SetParent(&chargeParents_[0]->GetTransform());

	ParticleManager::Load(roaringWave_, "roringWave");
	ParticleManager::Load(roaringParticle_, "roringParticle");
	ParticleManager::Load(roaringRing_, "roringring");
	roaringWave_.pos_ = animeModel_->GetTransform().translate;
	roaringWave_.grain_.isAutoUVMove_ = true;
	roaringWave_.grain_.autoUVSpeed_.x = 0.01f;
	roaringWave_.grain_.isZandX_ = true;
	animeModel_->RegisterJointWorld("mixamorig:Head");
	animeModel_->RegisterJointWorld("mixamorig:LeftHand");
	animeModel_->RegisterJointWorld("mixamorig:RightHand");
	roaringRing_.SetAnimeParent(animeModel_->GetJointTrans("mixamorig:Head"));
	roaringRing_.pos_.y = -7.0f;
	roaringParticle_.SetAnimeParent(animeModel_->GetJointTrans("mixamorig:Head"));
	roaringParticle_.pos_.y = -6.5f;
	roaringParticle_.pos_.z = 5.0f;

	ParticleManager::Load(dushStartParticle_, "dushChargeCompleteParticle");
	ParticleManager::Load(dushStartCircle_, "dushChargeCompleteCircle");
	ParticleManager::Load(dushSmoke_, "dushGroundSmoke");
	dushStartParticle_.frequencyTime_ = 0.0f;
	dushStartCircle_.frequencyTime_ = 0.0f;
	dushTrailIndex_ = ParticleManager::GetInstance()->InitGPUEmitter(1);
	ParticleManager::GetParticleCSEmitter(dushTrailIndex_).Load("DushTrail2");
}

void Boss::DushInit() {
	float parentRotate = std::numbers::pi_v<float> *0.25f;
	for (int i = 0; i < kChargeCount_; i++) {
		if (i != 0 && i != 4) {
			chargeParents_[i]->GetTransform().rotate.x = Random::GetFloat(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);
			chargeParents_[i]->GetTransform().rotate.y = Random::GetFloat(-std::numbers::pi_v<float>*0.5f, std::numbers::pi_v<float>*0.5f);
		}
		chargeParents_[i]->GetTransform().translate.y = 4.0f;
		chargeParents_[i]->GetTransform().rotate.z = parentRotate * i;
	}
	chargeTime_ = 70.0f;
	chargeSize_ = params_.beam.baseChargeSize;
	charge15_.grain_.startSize_ = { chargeSize_ * 3.0f,chargeSize_ * 6.0f };
	for (auto& chargeParent : chargeParents_) {
		chargeParent->GetTransform().scale = Vector3::FillVec(chargeSize_);
	}

	for (int i = 0; i < kChargeCount_; i++) {
		auto& emitter = ParticleManager::GetSphereEmitter(traceEmitterIndexes_[i]);
		emitter.SetEmit(true);
		emitter.GetData().lifeTime = 0.6f;
		UpdateEmitterPos(i);
		emitter.GetData().prevTranslate = traceAnchors_[i]->GetWorldPos();

		Vector3 randColor = Random::GetVector3({ 0.0f,0.0f }, { 0.2f,0.8f }, { 0.2f,1.0f });
		emitter.GetData().colorMax = randColor;
		emitter.GetData().colorMin = randColor;
	}
}

void Boss::InitBeam() {
	auto& bp = params_.beam;
	float parentRotate = bp.parentRotateStep;
	for (int i = 0; i < kChargeCount_; i++) {
		if (i != 0 && i != 4) {
			chargeParents_[i]->GetTransform().rotate.x = Random::GetFloat(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);
			chargeParents_[i]->GetTransform().rotate.y = Random::GetFloat(-std::numbers::pi_v<float>*0.5f, std::numbers::pi_v<float>*0.5f);
		}
		chargeParents_[i]->GetTransform().translate.y = bp.parentY;
		chargeParents_[i]->GetTransform().rotate.z = parentRotate * i;
	}

	chargeTime_ = bp.chargeTime;
	chargeSize_ = bp.baseChargeSize;
	charge15_.grain_.startSize_ = { chargeSize_ * bp.startSizeMulX,chargeSize_ * bp.startSizeMulY };

	for (auto& chargeParent : chargeParents_) {
		chargeParent->GetTransform().scale = Vector3::FillVec(chargeSize_);
	}

	for (int i = 0; i < kChargeCount_; i++) {
		auto& emitter = ParticleManager::GetSphereEmitter(traceEmitterIndexes_[i]);
		emitter.SetEmit(true);
		emitter.GetData().lifeTime = bp.emitterLifeTime;
		UpdateEmitterPos(i);
		emitter.GetData().prevTranslate = traceAnchors_[i]->GetWorldPos();

		Vector3 randColor = Random::GetVector3({ 0.3f,1.0f }, { 0.0f,0.2f }, { 0.0f,0.5f });
		emitter.GetData().colorMax = randColor;
		emitter.GetData().colorMin = randColor;
	}
}

void Boss::InitJumpAttack() {
	jumpTime_ = params_.jump.jumpTotalTime;
	isJumpAttack_ = true;
}

void Boss::InitSummon() {
	summonIndex_ = ParticleManager::GetInstance()->InitGPUEmitterTexture("magicCircle.png");

	auto& emitter = ParticleManager::GetParticleCSEmitterTexture(summonIndex_);
	emitter.SetEmit(true);
	emitter.GetData().lifeTime = 0.8f;
	emitter.GetData().radius = 10.0f;
	emitter.GetData().frequency = 0.01f;
	emitter.GetData().translate = animeModel_->GetTransform().translate;
	emitter.GetData().baseVelocity = { 0.0f,0.6f,0.0f };

	bossYPos_ = animeModel_->GetTransform().translate.y;
	defaultCorePos_ = core_->GetWorldPos();
	animeModel_->GetTransform().translate.y = -30.0f;

	ParticleManager::Load(summonLightning_, "summonLightning_");
	ParticleManager::Load(energySphere_, "energySphere");
	ParticleManager::Load(energyParticle_, "energyParticle");

	summonLightning_.grain_.isZandX_ = true;
	energySphere_.grain_.isZandX_ = true;

	summonLightning_.pos_ = animeModel_->GetTransform().translate;
	summonLightning_.pos_.y = 40.0f;
	energySphere_.pos_ = animeModel_->GetTransform().translate;
	energySphere_.pos_.y = 0.0f;
	energyParticle_.pos_ = animeModel_->GetTransform().translate;
	energyParticle_.pos_.y = 0.0f;

}

void Boss::InitChargeParent() {
	float parentRotate = std::numbers::pi_v<float> *0.25f;
	for (int i = 0; i < kChargeCount_; i++) {// チャージエミッターの親となるもの
		std::unique_ptr<Object3d> chargeParent;
		chargeParent = std::make_unique<Object3d>();
		chargeParent->Create("cube.obj");
		chargeParent->GetTransform().translate.y += 20.0f;
		chargeParent->GetTransform().scale.x = params_.beam.baseChargeSize;
		chargeParent->GetTransform().scale.y = params_.beam.baseChargeSize;
		chargeParent->GetTransform().scale.z = params_.beam.baseChargeSize;
		chargeParent->SetParent(&animeModel_->GetTransform());
		chargeParent->SetNoneScaleParent(true);
		if (i != 0 && i != 4) {
			if (i < 4) {
				chargeParent->GetTransform().rotate.x = Random::GetFloat(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);
				chargeParent->GetTransform().rotate.y = Random::GetFloat(-1.56f, 1.56f);
			} else {
				chargeParent->GetTransform().rotate.x = Random::GetFloat(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);
				chargeParent->GetTransform().rotate.y = Random::GetFloat(-1.56f, 1.56f);
			}
		}
		chargeParent->GetTransform().rotate.z = parentRotate * i;
		chargeParents_.push_back(std::move(chargeParent));
	}
	for (int i = 0; i < kChargeCount_; i++) {// ダッシュチャージエミッターの親となるもの
		std::unique_ptr<Object3d> anchor;
		anchor = std::make_unique<Object3d>();
		anchor->Create("cube.obj");
		anchor->SetParent(&chargeParents_[i]->GetTransform());
		anchor->SetNoneScaleParent(true);
		traceAnchors_.push_back(std::move(anchor));

		int numEmitter = ParticleManager::GetInstance()->InitGPUEmitter();
		auto& emitter = ParticleManager::GetSphereEmitter(numEmitter);
		emitter.SetEmit(false);
		emitter.GetData().colorMax = { 1.0f,0.0f,0.0f };
		emitter.GetData().colorMin = { 1.0f,0.0f,0.0f };
		emitter.GetData().frequency = 0.0f;
		emitter.GetData().radius = 0.0f;
		emitter.GetData().scale = { 1.5f,1.5f,1.5f };
		emitter.GetData().lifeTime = 0.6f;
		traceEmitterIndexes_.push_back(numEmitter);
	}
}

void Boss::InitItemParent() {
	waveParent_ = std::make_unique<Object3d>();
	waveParent_->Create("cube.obj");
	waveParent_->GetTransform().translate.z += 8.0f;
	waveParent_->SetParent(&animeModel_->GetTransform());
	waveParent_->SetNoneScaleParent(true);

	for (int i = 0; i < arrowSpawnNum_; i++) {
		std::unique_ptr<Object3d> arrowParent;

		arrowParent = std::make_unique<Object3d>();
		arrowParent->Create("cube.obj");

		arrowParent->GetTransform().translate.x = 12.0f - float(i) * 5.0f;
		if (i > 1) {
			arrowParent->GetTransform().translate.x = -12.0f + float(i - 2) * 5.0f;
		}
		arrowParent->GetTransform().translate.y += 6.0f;
		arrowParent->GetTransform().translate.z -= 2.0f;
		arrowParent->SetParent(&animeModel_->GetTransform());
		arrowParent->SetNoneScaleParent(true);

		arrowParents_.push_back(std::move(arrowParent));
	}
}