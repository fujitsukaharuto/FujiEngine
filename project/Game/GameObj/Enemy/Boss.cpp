#include "Boss.h"
#include <json.hpp>
#include "Engine/Graphics/Particle/ParticleManager.h"
#include "Engine/Math/Random/Random.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Core/Serialize/JsonSerializer.h"
#include <numbers>

#include "Game/Particle/GameEmitters.h"
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
#include "Engine/Graphics/PostEffect/OffscreenManager.h"

using namespace Audio;
using namespace Core;
using namespace Graphics;
using namespace Math;


Boss::Boss() {
}

Boss::~Boss() {
	ParticleManager::GetParticleCSEmitterTexture(summonIndex_).SetEmit(false);
	ParticleManager::GetParticleCSEmitter(dushTrailIndex_).SetEmit(false);
	ParticleManager::GetInstance()->GetSphereEmitter(waveCSEmitIndex_).SetEmit(false);
	ParticleManager::GetInstance()->GetSphereEmitter(jumpCSEmitIndex_).SetEmit(false);
	ParticleManager::GetSphereEmitter(halfAuraCS_).SetEmit(false);
	ParticleManager::GetSphereEmitter(halfSmallAuraCS_).SetEmit(false);
	ParticleManager::GetSphereEmitter(leftHandAuraCS_).SetEmit(false);
	ParticleManager::GetSphereEmitter(rightHandAuraCS_).SetEmit(false);

	StopTraceEmitters();
}

void Boss::Update() {
	if (!isDying_ && isHpActive_ && !isStart_) {
		behavior_->Update();

		core_->Update();

		beam_->Update();
		itemManager_->Update();

		ShakeHP();

	} else if (isStart_) { // 開始時の召喚処理
		if (EnergyUpdate()) {
			startTime_ -= FPSKeeper::DeltaTimeFrame();
			if (startTime_ > kRoarShakeTimeMin_ && startTime_ < kRoarShakeTimeMax_) {
				CameraManager::GetInstance()->GetCamera()->IssuanceShake(0.2f, 2.0f);
				roaringWave_.Emit();
				roaringParticle_.Emit();
				roaringRing_.Emit();
			}
			if (startTime_ < 0.0f) {
				isStart_ = false;
				isActiveSprite_ = true;
				ChangeBehavior(std::make_unique<BossRoot>(this));
			}
		}
	} else if (!isHpActive_) { // HPの段階が切り替わった際の処理
		hpCoolTime_ -= FPSKeeper::DeltaTimeFrame();
		RadialUpdate();

		itemManager_->Update();
		if (hpCoolTime_ < 0.0f) {
			isHpActive_ = true;
			ChangeBehavior(std::make_unique<BossRoot>(this));
			animeModel_->IsLoopAnimation(true);
		}
	} else {
		dyingTime_ -= FPSKeeper::DeltaTimeFrame();
		if (dyingTime_ < 0.0f) {
			isClear_ = true;
		}
	}

	ParticleManager::GetSphereEmitter(halfAuraCS_).SetPos({ animeModel_->GetTransform().translate.x,animeModel_->GetTransform().translate.y + params_.jump.height,animeModel_->GetTransform().translate.z });
	ParticleManager::GetSphereEmitter(halfSmallAuraCS_).SetPos({ animeModel_->GetTransform().translate.x,animeModel_->GetTransform().translate.y + params_.jump.height,animeModel_->GetTransform().translate.z });
	ParticleManager::GetSphereEmitter(leftHandAuraCS_).SetPos(animeModel_->GetJointWorldPos("mixamorig:LeftHand"));
	ParticleManager::GetSphereEmitter(rightHandAuraCS_).SetPos(animeModel_->GetJointWorldPos("mixamorig:RightHand"));
	animeModel_->AnimationUpdate();
	collider_->InfoUpdate();
}

void Boss::Draw([[maybe_unused]] bool is) {
	OriginGameObject::Draw();
	DrawColliders();

	core_->Draw();
	itemManager_->Draw();

	beam_->Draw();

	if (isActiveSprite_) {
		for (auto& hpTex : hpFrame_) {
			hpTex->Draw();
		}
		if (bossHp_ >= 0.0f) {
			int texCount = 0;
			for (auto& tex : hpSprites_) {
				tex->Draw();
				if (nowHpIndex_ == texCount) break;
				texCount++;
			}
		}
	}
}

void Boss::CSDispatch() {
	animeModel_->CSDispatch();
}

void Boss::AnimeDraw() {
	animeModel_->Draw();
}

void Boss::ReStart() {
	SetDefaultBehavior(true);
	isHpActive_ = true;
	isDying_ = false;
	isStart_ = true;
	isActiveSprite_ = false;
	isDamageLight_ = false;
	startTime_ = kStartTime_;
	animeModel_->ChangeAnimation("roaring");
	animeModel_->LoadTransformFromJson("boss_transform.json");
	animeModel_->GetTransform().rotate.y = 3.14f;
	animeModel_->IsLoopAnimation(true);
	animeModel_->ChangeAnimation("roaring");
}

void Boss::ReduceBossHP(bool isStrong) {
	if (isHpActive_) {
		if (isStrong) {
			bossHp_ -= kStrongDamage_;
		} else {
			bossHp_ -= kNormalDamage_;
		}
		isDamageLight_ = true;
		// フェーズやHPの段階を切り替える為の処理
		switch (BossHPState(nowHpIndex_)) {
		case BossHPState::Max:
			HPColorSet(kHpThresholdMax_, kHpColorRangeWide_);
			if (bossHp_ < kHpThresholdMax_) {
				ChangePhase(kHpThresholdMax_, 1);
				return;
			}
			break;
		case BossHPState::High:
			HPColorSet(kHpThresholdHigh_, kHpColorRangeWide_);
			if (bossHp_ < kHpThresholdHigh_) {
				ChangePhase(kHpThresholdHigh_, 1);
				phaseIndex_++;
				ParticleManager::GetSphereEmitter(halfAuraCS_).SetEmit(true);
				ParticleManager::GetSphereEmitter(halfSmallAuraCS_).SetEmit(true);
				ParticleManager::GetSphereEmitter(leftHandAuraCS_).SetEmit(true);
				ParticleManager::GetSphereEmitter(rightHandAuraCS_).SetEmit(true);
				return;
			}
			break;
		case BossHPState::Half:
			HPColorSet(kHpThresholdHalf_, kHpColorRangeWide_);
			if (bossHp_ < kHpThresholdHalf_) {
				ChangePhase(kHpThresholdHalf_, 1);
				return;
			}
			break;
		case BossHPState::Low:
			HPColorSet(kHpThresholdLow_, kHpColorRangeNarrow_);
			if (bossHp_ < kHpThresholdLow_) {
				ChangePhase(kHpThresholdLow_, 1);
				return;
			}
			break;
		case BossHPState::Empty:
			HPColorSet(0.0f, kHpColorRangeEmpty_);
			break;
		default:
			break;
		}
		if (nowHpIndex_ < 0) nowHpIndex_ = 0;
		isShakeSprite_ = true;
		shakeTime_ = baseShakeTime_;
		// ボスが死んだとき
		if (bossHp_ < 0.0f && !isDying_) {
			isDying_ = true;
			SetDefaultBehavior(true);
			animeModel_->ChangeAnimation("dying");
			ParticleManager::GetSphereEmitter(halfAuraCS_).SetEmit(false);
			ParticleManager::GetSphereEmitter(halfSmallAuraCS_).SetEmit(false);
			ParticleManager::GetSphereEmitter(leftHandAuraCS_).SetEmit(false);
			ParticleManager::GetSphereEmitter(rightHandAuraCS_).SetEmit(false);
		}
	}
}

void Boss::HPColorSet(float under, float index) {
	if (under + index * 0.75f < bossHp_) {
		hpSprites_[nowHpIndex_]->SetColor(damageColor1_);
	} else if (under + index * 0.5f < bossHp_) {
		hpSprites_[nowHpIndex_]->SetColor(damageColor2_);
	} else if (under + index * 0.25f < bossHp_) {
		hpSprites_[nowHpIndex_]->SetColor(damageColor3_);
	} else if (under + index * 0.0f < bossHp_) {
		hpSprites_[nowHpIndex_]->SetColor(damageColor4_);
	}
}

void Boss::ShakeHP() {
	if (isShakeSprite_) {
		// ダメージ喰らった際にHPバーの色を変えていく
		shakeTime_ -= FPSKeeper::DeltaTimeFrame();
		if (shakeTime_ < 0.0f) {
			shakeTime_ = 0.0f;
			isShakeSprite_ = false;
		}
		float t = shakeTime_ / baseShakeTime_; // 1.0 → 0.0 に減る
		float theta = t * 2.0f * std::numbers::pi_v<float>; // sin/cos の周期は π（0→π）

		float offsetX = std::cos(theta + std::numbers::pi_v<float> / 2.0f) * shakeSize_;
		float offsetY = std::sin(theta + std::numbers::pi_v<float>) * shakeSize_;
		hpSprites_[nowHpIndex_]->SetPos({ hpStartPos_.x + (hpSize_.x * nowHpIndex_) + (hpIndent * nowHpIndex_) + offsetX, hpStartPos_.y + offsetY, 0.0f });
	}
}

void Boss::Walk() {
	if (BossRoot* behavior = dynamic_cast<BossRoot*>(behavior_.get())) {
		Vector3 dir = pPlayer_->GetWorldPos() - animeModel_->GetTransform().translate;
		dir.y = 0.0f; // 水平方向だけに限定
		dir = dir.Normalize();

		// 目標のY軸角度（ラジアン）
		float targetAngle = std::atan2(dir.x, dir.z); // Z前方軸に対する角度

		Vector3 front = GetFrontOffset(Vector3(0.0f, 0.0f, 1.0f) * kWalkSpeed_ * FPSKeeper::DeltaTimeFrame(), targetAngle);
		animeModel_->GetTransform().translate += front;

		CalcModelDir();
	}
}

void Boss::CalcModelDir() {
	Vector3 dir = pPlayer_->GetWorldPos() - animeModel_->GetTransform().translate;
	dir.y = 0.0f; // 水平方向だけに限定
	dir = dir.Normalize();

	// 目標のY軸角度（ラジアン）
	float targetAngle = std::atan2(dir.x, dir.z); // Z前方軸に対する角度
	// 現在のY軸角度（モデルの回転）
	float currentAngle = animeModel_->GetTransform().rotate.y;

	// 角度差を -π〜+π にラップ
	float delta = targetAngle - currentAngle;
	if (delta > std::numbers::pi_v<float>) delta -= 2.0f * std::numbers::pi_v<float>;
	if (delta < -std::numbers::pi_v<float>) delta += 2.0f * std::numbers::pi_v<float>;

	// 角度補間（例えば線形補間）
	float lerpFactor = 0.1f; // 追従の速さ
	float newAngle = currentAngle + delta * lerpFactor;

	animeModel_->GetTransform().rotate.y = newAngle;
}

bool Boss::DushCharge(float& t, float maxT, bool& isNear, float range) {
	if (t <= 0.0f) {
		DushInit();
	}
	if (t <= maxT * 0.75f) {
		CalcModelDir();
	}
	t += FPSKeeper::DeltaTimeFrame();

	for (int i = 0; i < kChargeCount_; i++) {
		auto& emitter = TraceEmitter(i);
		emitter.GetData().prevTranslate = traceAnchors_[i]->GetWorldPos();

		if (chargeParents_[i]->scale.x > 0.0f) { // チャージのサイズを縮小していく
			ShrinkScale(i, chargeSize_ / chargeTime_);
			chargeParents_[i]->rotate.z += 0.05f * FPSKeeper::DeltaTimeFrame();
		}
		UpdateEmitterPos(i);
	}

	if (Vector3(pPlayer_->GetWorldPos() - animeModel_->GetTransform().translate).Length() < range) {
		isNear = true;
	} else {
		isNear = false;
	}
	if (t > maxT) { // ダッシュのチャージ時間が超えているか
		Vector3 emitPos = GetFrontOffset(Vector3(0.0f, 0.0f, 8.5f), animeModel_->GetTransform().rotate.y);
		emitPos += animeModel_->GetTransform().translate;
		emitPos.y = 4.0f;
		dushStartParticle_.pos_ = emitPos;
		dushStartCircle_.pos_ = emitPos;
		dushStartParticle_.Emit();
		dushStartCircle_.Emit();
		StopTraceEmitters();
		return true;
	}
	return false;
}

bool Boss::DushAttack(bool isNear, float& dushRange, float stopRange) {
	if (dushRange == 0.0f) {
		ParticleManager::GetParticleCSEmitter(dushTrailIndex_).SetEmit(true);
		ParticleManager::GetParticleCSEmitter(dushTrailIndex_).SetPos(animeModel_->GetTransform().translate);
	}
	isNowDush_ = true;
	Vector3 front = GetFrontOffset(Vector3(0.0f, 0.0f, 2.5f), animeModel_->GetTransform().rotate.y);
	animeModel_->GetTransform().translate += front;
	dushRange += front.Length();
	Vector3 emitPos = GetFrontOffset(Vector3(0.0f, 0.0f, 8.5f), animeModel_->GetTransform().rotate.y);
	emitPos += animeModel_->GetTransform().translate;
	emitPos.y = 3.0f;
	ParticleManager::GetParticleCSEmitter(dushTrailIndex_).SetPos(emitPos);
	emitPos = GetFrontOffset(Vector3(0.0f, 0.0f, 5.5f), animeModel_->GetTransform().rotate.y);
	emitPos += animeModel_->GetTransform().translate;
	emitPos.y = 1.0f;
	dushSmoke_.pos_ = emitPos;
	dushSmoke_.Emit();
	if (!isNear && dushRange >= stopRange * 1.5f) {// 距離に応じて変える
		ParticleManager::GetParticleCSEmitter(dushTrailIndex_).SetEmit(false);
		isNowDush_ = false;
		return true;
	} else if (isNear && dushRange >= stopRange) {
		ParticleManager::GetParticleCSEmitter(dushTrailIndex_).SetEmit(false);
		isNowDush_ = false;
		return true;
	}
	return false;
}

void Boss::WaveWallAttack() {
	Vector3 wavePos = GetFrontPos();
	itemManager_->WaveWallAttack(wavePos, animeModel_->GetTransform().rotate.y);

	waveAttack1.Emit();
	waveAttack2.Emit();
	waveAttack3.Emit();
	waveAttack4.Emit();
	ParticleManager::GetInstance()->GetSphereEmitter(waveCSEmitIndex_).SetPos(wavePos);
	ParticleManager::GetInstance()->GetSphereEmitter(waveCSEmitIndex_).Emit();
}

void Boss::ArrowAttack() {
	std::vector<Vector3> pos;
	for (int i = 0; i < arrowSpawnNum_; i++) {
		pos.push_back(arrowParents_[i]->GetWorldPos());
	}
	itemManager_->ArrowAttack(pos,arrowSpawnNum_);
}

void Boss::RodFall() {
	itemManager_->RodFall(animeModel_->GetTransform().translate);
}

bool Boss::BeamCharge() {
	auto& bp = params_.beam;
	bool result = false;
	if (Game::BeamCrystalEmitter().IsEmit() == false) {
		auto& emitter = Game::BeamCrystalEmitter();
		emitter.SetEmit(true);
		emitter.GetData().radius = 0.0f;
		emitter.GetData().translate = animeModel_->GetTransform().translate;
		emitter.GetData().translate.y = bp.parentY;
	}

	if (chargeSize_ > 0.0f) {
		Game::BeamCrystalEmitter().GetData().radius += bp.radiusGrowSpeed * FPSKeeper::DeltaTimeFrame();
		for (int i = 0; i < kChargeCount_; i++) {
			if (i > 2) {
				if (!(chargeTime_ < bp.chargeTime - i * 2.0f)) {
					continue;
				}
			}

			auto& emitter = TraceEmitter(i);
			emitter.GetData().prevTranslate = traceAnchors_[i]->GetWorldPos();

			if (chargeParents_[i]->scale.x > 0.0f) { // チャージのサイズを縮小していく
				ShrinkScale(i, bp.shrinkSpeed * FPSKeeper::DeltaTimeFrame());
				chargeParents_[i]->rotate.z += bp.rotateSpeedZ * FPSKeeper::DeltaTimeFrame();
			}
			if (chargeParents_[i]->scale.x <= 0.0f) {
				if (i == 0) {
					chargeSize_ -= bp.sizeDecrease;
					if (chargeSize_ <= 0.0f) {
						chargeSize_ = 0.0f;
					}
				}
				chargeParents_[i]->scale = Vector3::FillVec(chargeSize_);
				float emitPos = chargeParents_[i]->scale.x;
				traceAnchors_[i]->translate = { emitPos,emitPos,emitPos };

				emitter.GetData().prevTranslate = traceAnchors_[i]->GetWorldPos();
				emitter.GetData().lifeTime -= bp.lifeTimeDecrease;
			}
			UpdateEmitterPos(i);
		}

		if (chargeSize_ > bp.minReCalcSize) {// チャージサイズを少し小さく
			charge15_.grain_.startSize_ = { chargeSize_ * bp.startSizeMulX,chargeSize_ * bp.startSizeMulY };
		}

		charge9_.Emit();
		charge10_.Emit();
		charge11_.Emit();
		charge15_.Emit();
		chargeTime_ -= FPSKeeper::DeltaTimeFrame();
	} else if (chargeSize_ <= 0.0f) {
		result = true;
		for (int i = 0; i < kChargeCount_; i++) {
			if (chargeParents_[i]->scale.x > 0.0f) { // 完全に真ん中に集結するようにする
				auto& emitter = TraceEmitter(i);
				emitter.GetData().prevTranslate = traceAnchors_[i]->GetWorldPos();

				ShrinkScale(i, bp.shrinkSpeed * FPSKeeper::DeltaTimeFrame());
				chargeParents_[i]->rotate.z += bp.rotateSpeedZ * FPSKeeper::DeltaTimeFrame();
				result = false;
			}
			if (!result) {
				UpdateEmitterPos(i);
			}
		}
	}

	if (result) {
		chargeParents_[0]->scale = { 1.0f,1.0f,1.0f };
	}

	return result;
}

void Boss::BeamChargeComplete() {
	StopTraceEmitters();
	charge12_.Emit();
	charge13_.Emit();
	charge14_.Emit();
	beam_->InitBeam(Vector3(), Vector3());
}

bool Boss::BeamAttack() {
	bool result = false;

	if (beam_->BeamAttackUpdate()) {
		result = true;
	}

	return result;
}

bool Boss::JumpAttack() {

	if (jumpTime_ > 0.0f) {
		jumpTime_ -= FPSKeeper::DeltaTimeFrame();
		if (jumpTime_ < 0.0f) {
			jumpTime_ = 0.0f;
		}
	}
	auto& jp = params_.jump;

	// 時間でジャンプの挙動を制御する
	if (jumpTime_ <= jp.upStart && jumpTime_ >= jp.upEnd) {//120~90 //30

		float upTime = jp.upStart - jp.upEnd;
		float flyT = 1.0f - ((jumpTime_ - jp.upEnd) / upTime);
		animeModel_->GetTransform().translate.y = std::lerp(0.0f, jp.height, (1.0f - (1.0f - flyT) * (1.0f - flyT)));
	} else if (jumpTime_ < jp.downStart && jumpTime_ >= jp.downEnd) {//70~50 //20

		float downTime = jp.downStart - jp.downEnd;
		float flyT = 1.0f - (jumpTime_ - jp.downEnd) / downTime;
		animeModel_->GetTransform().translate.y = std::lerp(jp.height, 0.0f, (1.0f - powf(1.0f - flyT, 4.0f)));
		if (std::abs(animeModel_->GetTransform().translate.y) < jp.groundJudgeHeight) {
			if (isJumpAttack_) {
				jumpWave_.pos_ = animeModel_->GetTransform().translate;
				jumpWave_.Emit();
				ParticleManager::GetSphereEmitter(jumpCSEmitIndex_).SetPos(jumpWave_.pos_);
				ParticleManager::GetSphereEmitter(jumpCSEmitIndex_).Emit();
				AudioPlayer::GetInstance()->SoundPlayWave(*jumpAttackSE_, 0.6f);
				UnderRingEmit();
			}
		}
	} else if (jumpTime_ <= 0.0f) {
		animeModel_->GetTransform().translate.y = 0.0f;
		return true;
	}

	return false;
}

void Boss::UnderRingEmit() {
	isJumpAttack_ = false;
	itemManager_->UnderRingEmit(animeModel_->GetTransform().translate);
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

bool Boss::GetIsDamageLight() {
	bool result = isDamageLight_;
	if (isDamageLight_) {
		isDamageLight_ = !isDamageLight_;
	}
	return result;
}

float Boss::GetChainRate() {
	return chainRate_ + (chainCount_ - 1) * kChainRateStep_;
}

Math::Vector3 Boss::GetFrontPos() {
	Vector3 frontP = GetFrontOffset(Vector3(0.0f, 0.0f, frontZ_), animeModel_->GetTransform().rotate.y);
	frontP += animeModel_->GetTransform().translate;
	return frontP;
}

Math::Vector3 Boss::GetDamageLightPos() {
	Vector3 damageP = GetFrontOffset(Vector3(0.0f, 0.0f, frontZ_), animeModel_->GetTransform().rotate.y);
	damageP += animeModel_->GetTransform().translate;
	damageP.y += bossYPos_ + coreY_;
	return damageP;
}

void Boss::SavePhase() {
	json json;

	for (int phase = 0; phase < phaseList_.size(); ++phase) {
		json["Phases"][phase] = json::array();
		for (const auto& action : phaseList_[phase]) {
			json["Phases"][phase].push_back({
				{ "name", action.first },
				{ "value", action.second }
				});
		}
	}

	std::filesystem::create_directories("resource/Json/Phase/");
	std::filesystem::path fullPath = "resource/Json/Phase/boss_phase.json";

	JsonSerializer::SerializeJsonData(json, fullPath.string());
}

void Boss::LoadPhase() {
	json json = JsonSerializer::DeserializeJsonData("resource/Json/Phase/boss_phase.json");

	// 一度中身をクリア
	phaseList_.clear();

	if (json.contains("Phases") && json["Phases"].is_array()) {
		for (const auto& actions : json["Phases"]) {
			std::vector<std::pair<std::string, float>> phase;

			for (const auto& action : actions) {
				if (action.contains("name") && action.contains("value")) {
					std::string name = action["name"];
					float value = action["value"];
					phase.emplace_back(name, value);
				}
			}

			phaseList_.push_back(phase);
		}
	}
}

void Boss::SetDefaultBehavior(bool isInvisibleItem) {
	beam_->SetIsLive(false);
	StopTraceEmitters();
	ParticleManager::GetParticleCSEmitter(dushTrailIndex_).SetEmit(false);
	if (isInvisibleItem) {
		itemManager_->ReStart();
	}
	cameraRange_ = kCameraRange_;
	cameraFollowSpeed_ = kCameraFollowSpeed_;
	ChangeBehavior(std::make_unique<BossRoot>(this));
	animeModel_->GetTransform().translate.y = 0.0f;
	Vector3 dir = pPlayer_->GetWorldPos() - animeModel_->GetTransform().translate;
	dir.y = 0.0f; // 水平方向だけに限定
	dir = dir.Normalize();
	// 目標のY軸角度（ラジアン）
	float targetAngle = std::atan2(dir.x, dir.z); // Z前方軸に対する角度
	animeModel_->GetTransform().translate += GetFrontOffset(Vector3(0.0f, 0.0f, 1.0f) * kWalkSpeed_ * FPSKeeper::DeltaTimeFrame(), targetAngle);
	// 現在のY軸角度（モデルの回転）
	float currentAngle = animeModel_->GetTransform().rotate.y;
	// 角度差を -π〜+π にラップ
	float delta = targetAngle - currentAngle;
	if (delta > std::numbers::pi_v<float>) delta -= 2.0f * std::numbers::pi_v<float>;
	if (delta < -std::numbers::pi_v<float>) delta += 2.0f * std::numbers::pi_v<float>;
	float newAngle = currentAngle + delta;
	animeModel_->GetTransform().rotate.y = newAngle;
}

void Boss::RadialSetting() {
	radialTime_ = params_.radial.baseTime;
	dxcommon_->GetOffscreenManager()->AddPostEffect(PostEffectList::Radial);
	dxcommon_->GetOffscreenManager()->SetRadialParamsWidth(params_.radial.widthStart);
}

void Boss::RadialUpdate() {
	if (radialTime_ > 0.0f) {
		radialTime_ -= FPSKeeper::DeltaTimeFrame();
		if (radialTime_ <= 0.0f) {
			radialTime_ = 0.0f;
			dxcommon_->GetOffscreenManager()->PopPostEffect(PostEffectList::Radial);
		}
		auto& rp = params_.radial;
		float t = radialTime_ / rp.baseTime;
		float radialWidth = std::lerp(rp.widthStart, rp.widthEnd, 1.0f - powf(1.0f - t, 3.0f));
		dxcommon_->GetOffscreenManager()->SetRadialParamsWidth(radialWidth);
	}
}

bool Boss::EnergyUpdate() {
	ExpandSummon();
	EnergyTimeUpdate();
	bool result = false;
	if (energyCoolTime_ <= 0.0f) {
		result = true;
	}
	return result;
}

void Boss::ExpandSummon() {
	if (FPSKeeper::DeltaTimeFrame() > FPSKeeper::GetClampFrame()) return;
	if (startWaiting_ > 0.0f) {
		startWaiting_ -= FPSKeeper::DeltaTimeFrame();
		return;
	}
	if (summonCircleExpandTime_ > 0.0f) { // 召喚陣の拡大
		isSummon_ = true;
		auto& sp = params_.summon;

		summonCircleExpandTime_ -= FPSKeeper::DeltaTimeFrame();
		float t = (std::max)(summonCircleExpandTime_ / sp.summonExpandTime, 0.0f);
		float summonRadius = std::lerp(sp.summonRadiusStart, sp.summonRadiusEnd, t);
		ParticleManager::GetParticleCSEmitterTexture(summonIndex_).GetData().radius = summonRadius;
		energyParticle_.Emit();
	}
}

void Boss::EnergyTimeUpdate() {
	if (summonCircleExpandTime_ > 0.0f) return;
	if (energyTime_ > 0.0f) { // 召喚時の更新処理
		auto& sp = params_.summon;
		if (energyTime_ >= sp.energySphereEmitStart) {
			energySphere_.Emit();
		}
		energyTime_ -= FPSKeeper::DeltaTimeFrame();
		if (energyTime_ >= sp.bossRiseStartTime) { // ボスの位置更新
			float t = (std::max)((energyTime_ - sp.bossRiseStartTime) / sp.bossRiseStartTime, 0.0f);
			animeModel_->GetTransform().translate.y = std::lerp(bossYPos_, sp.bossDownPos, t);
			summonCameraRotate_.x = std::lerp(summonCameraRotateStart_, summonCameraRotateEnd_, 1.0f - t);
		}
		energyParticle_.Emit();

		summonLightning_.particleRotate_.x = Random::GetFloat(-0.15f, 0.15f);
		summonLightning_.particleRotate_.y = Random::GetFloat(-3.14f, 3.14f);
		summonLightning_.particleRotate_.z = Random::GetFloat(-0.15f, 0.15f);
		summonLightning_.Emit();
	}
	if (summonCircleExpandTime_ > 0.0f || energyTime_ > 0.0f) return;
	if (energyCoolTime_ > 0.0f) {
		energyCoolTime_ -= FPSKeeper::DeltaTimeFrame();
		if (energyCoolTime_ <= 0.0f) {
			isSummon_ = false;
			animeModel_->GetTransform().translate.y = bossYPos_;
			animeModel_->ChangeAnimation("roaring");
			ParticleManager::GetParticleCSEmitterTexture(summonIndex_).SetEmit(false);
		}
		summonLightning_.particleRotate_.x = Random::GetFloat(-0.15f, 0.15f);
		summonLightning_.particleRotate_.y = Random::GetFloat(-3.14f, 3.14f);
		summonLightning_.particleRotate_.z = Random::GetFloat(-0.15f, 0.15f);
		summonLightning_.Emit();
	}
}

SphereEmitter& Boss::TraceEmitter(int i) {
	return ParticleManager::GetSphereEmitter(traceEmitterIndexes_[i]);
}

void Boss::StartTraceEmitters(float lifeTime, const Vector2& colorX, const Vector2& colorY, const Vector2& colorZ) {
	for (int i = 0; i < kChargeCount_; i++) {
		auto& emitter = TraceEmitter(i);
		emitter.SetEmit(true);
		emitter.GetData().lifeTime = lifeTime;
		UpdateEmitterPos(i);
		emitter.GetData().prevTranslate = traceAnchors_[i]->GetWorldPos();

		Vector3 randColor = Random::GetVector3(colorX, colorY, colorZ);
		emitter.GetData().colorMax = randColor;
		emitter.GetData().colorMin = randColor;
	}
}

void Boss::StopTraceEmitters() {
	for (int i = 0; i < kChargeCount_; i++) {
		TraceEmitter(i).SetEmit(false);
	}
}

void Boss::UpdateEmitterPos(int i) {
	float emitPos = chargeParents_[i]->scale.x;
	traceAnchors_[i]->translate = { emitPos, emitPos, emitPos };
	TraceEmitter(i).GetData().translate = traceAnchors_[i]->GetWorldPos();
}

void Boss::ShrinkScale(int i, float delta) {
	auto& s = chargeParents_[i]->scale;
	s.x = (std::max)(0.0f, s.x - delta);
	s.y = (std::max)(0.0f, s.y - delta);
	s.z = (std::max)(0.0f, s.z - delta);
}

void Boss::ChangePhase(float threshold, int indexInc) {
	bossHp_ = threshold;
	nowHpIndex_ -= indexInc;
	isHpActive_ = false;
	SetDefaultBehavior();
	animeModel_->ChangeAnimation("hit");
	animeModel_->IsLoopAnimation(false);
	RadialSetting();
	hpCoolTime_ = kHpCoolTime_;
	hpSprites_[nowHpIndex_]->SetColor(damageColor1_);
}

Vector3 Boss::GetFrontOffset(const Vector3& distance, float angle) {
	Vector3 front = distance;
	Matrix4x4 rot = MakeRotateYMatrix(angle);
	return TransformNormal(front, rot);
}
