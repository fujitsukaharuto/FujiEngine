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

BossRoot::BossRoot(Boss* pBoss) : BaseBossBehavior(pBoss) {
	step_ = Step::ROOT;
	cooldown_ = pBoss_->GetAttackCooldown();
	pBoss_->GetAnimModel()->ChangeAnimation("walk");
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
			cooldown_ -= FPSKeeper::DeltaTime();
			pBoss_->Walk();
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
			pBoss_->ChangeBehavior(std::make_unique<BossBeamAttack>(pBoss_));
			break;
		case AttackPattern::Wave:
			pBoss_->ChangeBehavior(std::make_unique<BossAttack>(pBoss_));
			break;
		case AttackPattern::JumpAttack:
			pBoss_->ChangeBehavior(std::make_unique<BossJumpAttack>(pBoss_, 2));
			break;
		case AttackPattern::SwordAttack:
			pBoss_->ChangeBehavior(std::make_unique<BossSwordAttack>(pBoss_));
			break;
		case AttackPattern::AreaAttack:
			pBoss_->ChangeBehavior(std::make_unique<BossAreaAttack>(pBoss_));
			break;
		case AttackPattern::ArrowAttack:
			pBoss_->ChangeBehavior(std::make_unique<BossArrowAttack>(pBoss_));
			break;
		case AttackPattern::FallRod:
			pBoss_->ChangeBehavior(std::make_unique<BossRodFall>(pBoss_));
			break;
		}
		break;
	}
	default:
		break;
	}

}

void BossRoot::Debug() {
}

AttackPattern BossRoot::ChooseNextAttack() {
	static AttackPattern previous = AttackPattern::Beam;

	std::vector<std::pair<std::string, float>> actions = pBoss_->GetPhaseActionList(pBoss_->GetPhaseIndex());
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
	throw std::invalid_argument("未知のAttackPattern名: " + name);
}
