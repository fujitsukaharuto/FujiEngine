#include "BossRoot.h"
#include "Engine/Math/Random/Random.h"

#include "Game/GameObj/Enemy/Boss.h"
#include "BossAttack.h"
#include "BossBeamAttack.h"
#include "BossJumpAttack.h"
#include "BossSwordAttack.h"
#include "BossAreaAttack.h"
#include "BossArrowAttack.h"
#include "BossRodFall.h"
#include "BossDushAttack.h"
#include "Engine/Core/Time/FPSKeeper.h"

using namespace Core;
using namespace Graphics;
using namespace Math;


BossRoot::BossRoot(Boss* pBoss) : BaseBossBehavior(pBoss) {
	step_ = Step::ROOT;
	owner_->SetCameraRange(cameraRange_);
	owner_->SetCameraFollowSpeed(cameraFollowSpeed_);
	cooldown_ = owner_->GetAttackCooldown();
	owner_->GetAnimeModel()->ChangeAnimation("walk");
	owner_->GetAnimeModel()->IsLoopAnimation(true);
	owner_->ResetChainCount();
}

BossRoot::~BossRoot() {
}

void BossRoot::Update() {

	switch (step_) {
		///---------------------------------------------------------------------------------------
		/// 通常
		///---------------------------------------------------------------------------------------
	case BossRoot::Step::ROOT:
		if (cooldown_ > 0.0f) {
			cooldown_ -= FPSKeeper::DeltaTimeFrame();
			owner_->Walk();
		} else if (cooldown_ <= 0.0f) {
			step_ = Step::TOATTACK;
			break;
		}


		break;
		///---------------------------------------------------------------------------------------
		/// 攻撃へ移行
		///---------------------------------------------------------------------------------------
	case BossRoot::Step::TOATTACK: {
		AttackPattern pattern = ChooseNextAttack();
		switch (pattern) {
		case AttackPattern::Beam:
			owner_->ChangeBehavior(std::make_unique<BossBeamAttack>(owner_));
			break;
		case AttackPattern::Wave:
			owner_->ChangeBehavior(std::make_unique<BossAttack>(owner_));
			break;
		case AttackPattern::JumpAttack:
		{
			int count = 2; // ジャンプ攻撃の回数は確率かフェーズによって増える
			if (owner_->GetPhaseIndex() > 0 || Random::GetFloat(0.0f, 1.0f) > 0.75f) count = 3;
			owner_->ChangeBehavior(std::make_unique<BossJumpAttack>(owner_, count));
		}
			break;
		case AttackPattern::SwordAttack:
			owner_->ChangeBehavior(std::make_unique<BossSwordAttack>(owner_));
			break;
		case AttackPattern::AreaAttack:
			owner_->ChangeBehavior(std::make_unique<BossAreaAttack>(owner_));
			break;
		case AttackPattern::ArrowAttack:
			owner_->ChangeBehavior(std::make_unique<BossArrowAttack>(owner_));
			break;
		case AttackPattern::FallRod:
			owner_->ChangeBehavior(std::make_unique<BossRodFall>(owner_));
			break;
		case AttackPattern::Dush:
			owner_->ChangeBehavior(std::make_unique<BossDushAttack>(owner_));
			break;
		}
		break;
	}
	default:
		break;
	}

}

AttackPattern BossRoot::ChooseNextAttack() {
	static AttackPattern previous = AttackPattern::Beam;

	std::vector<std::pair<std::string, float>> actions = owner_->GetPhaseActionList(owner_->GetPhaseIndex());
	std::vector<AttackInfo> patterns;

	for (const auto& action : actions) {
		patterns.push_back({ ToAttackPattern(action.first), action.second });
	}

	// 同じ攻撃を避けるための重み調整
	for (auto& info : patterns) {
		if (info.pattern == previous) {
			info.weight *= 0.2f; // 同じなら重みを下げる
		}
	}

	// 重みに基づいてランダム選択
	float totalWeight = 0.0f;
	for (const auto& info : patterns) totalWeight += info.weight;

	float r = Random::GetFloat(0.0f, totalWeight);
	float acc = 0.0f;
	for (const auto& info : patterns) {
		acc += info.weight;
		if (r <= acc) {
			previous = info.pattern;
			return info.pattern;
		}
	}

	// 万が一失敗したら最初を返す
	return patterns.front().pattern;
}

AttackPattern BossRoot::ToAttackPattern(const std::string& name) {
	if (name == "Beam") return AttackPattern::Beam;
	if (name == "Wave") return AttackPattern::Wave;
	if (name == "Jump") return AttackPattern::JumpAttack;
	if (name == "Sword") return AttackPattern::SwordAttack;
	if (name == "Area") return AttackPattern::AreaAttack;
	if (name == "Arrow") return AttackPattern::ArrowAttack;
	if (name == "FallRod") return AttackPattern::FallRod;
	if (name == "Dash") return AttackPattern::Dush;
	throw std::invalid_argument("未知のAttackPattern名: " + name);
}
