#include "BossItemManager.h"
#include "Game/GameObj/Enemy/Boss.h"
#include "Game/GameObj/Player/Player.h"
#include "Engine/Particle/ParticleManager.h"
#include "Engine/Math/Random/Random.h"
#include "Engine/Camera/CameraManager.h"
#include "Engine/Editor/JsonSerializer.h"
#include <numbers>


using namespace Audio;
using namespace Core;
using namespace Graphics;
using namespace Math;


BossItemManager::BossItemManager() {}
BossItemManager::~BossItemManager() {}

void BossItemManager::Initialize(Boss* boss) {
	pBoss_ = boss;
	

	for (int i = 0; i < 9; i++) {
		std::unique_ptr<WaveWall> wall;
		wall = std::make_unique<WaveWall>();
		wall->Initialize();
		walls_.push_back(std::move(wall));
	}
	for (int i = 0; i < 10; i++) {
		std::unique_ptr<Arrow> arrow;
		arrow = std::make_unique<Arrow>();
		arrow->Initialize();
		int numEmitter = ParticleManager::GetInstance()->InitGPUEmitter();
		arrow->SetEmitterNumber(numEmitter);
		arrow->GPUEmitterSetting();
		arrow->SetArrowMode();
		arrows_.push_back(std::move(arrow));

		std::unique_ptr<Arrow> rod;
		rod = std::make_unique<Arrow>();
		rod->Initialize();
		rods_.push_back(std::move(rod));

		std::unique_ptr<UnderRing> ring;
		ring = std::make_unique<UnderRing>();
		ring->Initialize();
		underRings_.push_back(std::move(ring));
	}
}

void BossItemManager::Update() {
	UpdateWaveWall();
	UpdateArrows();
	UpdateRod();
	UpdateUnderRing();
}

void BossItemManager::Draw() {
	for (auto& wall : walls_) {
		if (!wall->GetIsLive())continue;
		wall->Draw();
#ifdef _DEBUG
		wall->DrawCollider();
#endif // _DEBUG
	}

	for (auto& arrow : arrows_) {
		if (!arrow->GetIsLive())continue;
		arrow->Draw();
#ifdef _DEBUG
		arrow->DrawCollider();
#endif // _DEBUG
	}
	for (auto& rod : rods_) {
		if (!rod->GetIsLive())continue;
		rod->Draw();
	}

	for (auto& ring : underRings_) {
		if (!ring->GetIsLive())continue;
		ring->Draw();
#ifdef _DEBUG
		ring->DrawCollider();
#endif // _DEBUG
	}
}

void BossItemManager::ReStart() {
	for (auto& wave : walls_) {
		wave->SetIsLive(false);
	}
	for (auto& arrow : arrows_) {
		arrow->SetIsLive(false);
	}
	for (auto& rod : rods_) {
		rod->SetIsLive(false);
	}
	for (auto& ring : underRings_) {
		ring->SetIsLive(false);
	}
}

void BossItemManager::ClearAll() {
	walls_.clear();
	arrows_.clear();
	rods_.clear();
	underRings_.clear();
}

void BossItemManager::UpdateWaveWall() {
	for (auto& wall : walls_) {
		if (!wall->GetIsLive())continue;
		wall->CalculationFollowVec(pPlayer_->GetWorldPos());
		wall->Update();
	}
}

void BossItemManager::WaveWallAttack(const Vector3& pos, float RotateY) {
	int count = 0;
	CameraManager::GetInstance()->GetCamera()->IssuanceShake(0.2f, 15.0f);

	// 波攻撃の出現位置を決める
	Vector3 wavePos = pos;
	wavePos.y = 0.0f;
	for (auto& wall : walls_) {
		if (count == 3) break;
		if (wall->GetIsLive()) continue;
		Vector3 velocity = { 0.0f,0.0f,1.0f };
		if (count == 1) velocity = Vector3(-1.0f, 0.0f, 1.0f).Normalize();
		if (count == 2) velocity = Vector3(1.0f, 0.0f, 1.0f).Normalize();
		Vector3 front = velocity;
		Matrix4x4 rot = MakeRotateYMatrix(RotateY);
		velocity = TransformNormal(front, rot);

		wall->InitWave(wavePos, velocity);
		count++;
	}
}

void BossItemManager::UpdateArrows() {
	for (auto& arrow : arrows_) {
		if (!arrow->GetIsLive())continue;
		arrow->TargetSetting(pPlayer_->GetWorldPos());
		arrow->Update();
	}
}

void BossItemManager::ArrowAttack(const std::vector<Vector3>& pos, int spawnNum) {
	int count = 0;

	for (auto& arrow : arrows_) {
		if (count == spawnNum) break;
		if (arrow->GetIsLive()) continue;

		// 矢の位置と飛ぶまでの時間を決める
		Vector3 arrowPos = pos[count];
		float emitTime = 50.0f;
		if (count == 0) emitTime = 50.0f;
		if (count == 1) emitTime = 80.0f;
		if (count == 2) emitTime = 110.0f;
		if (count == 3) emitTime = 140.0f;
		arrow->InitArrow(arrowPos, emitTime);
		count++;
	}
}

void BossItemManager::UpdateRod() {
	for (auto& rod : rods_) {
		if (rod->GetIsBroke()) RodUnderRing(rod->GetWorldPos());
		if (!rod->GetIsLive())continue;
		rod->RodUpdate();
	}
}

void BossItemManager::RodFall(const Vector3& pos) {
	int count = 0;

	for (auto& rod : rods_) {
		if (count == 6) break;
		if (rod->GetIsLive()) continue;

		// ロッドの出現位置を決める
		Vector3 rodPos = pos;
		rodPos.x += Random::GetFloat(-60.0f, 60.0f);
		rodPos.y = 20.0f;
		rodPos.z += Random::GetFloat(-60.0f, 60.0f);
		float emittTime = 140.0f;
		rod->InitRod(rodPos, emittTime);
		count++;
	}
}

void BossItemManager::RodUnderRing(const Vector3& emitPos) {
	int count = 0;
	for (auto& ring : underRings_) {
		if (count == 1) break;
		if (ring->GetIsLive()) continue;
		ring->InitRing(emitPos, 120.0f);
		count++;
	}
}

void BossItemManager::UpdateUnderRing() {
	for (auto& ring : underRings_) {
		if (!ring->GetIsLive())continue;
		ring->Update();
	}
}

void BossItemManager::UnderRingEmit(const Vector3& pos) {
	CameraManager::GetInstance()->GetCamera()->IssuanceShake(0.3f, 30.0f);

	int count = 0;
	for (auto& ring : underRings_) {
		if (count == 1) break;
		if (ring->GetIsLive()) continue;
		ring->InitRing(pos);
		count++;
	}
}