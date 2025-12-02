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


Boss::Boss() {
}

Boss::~Boss() {
	ParticleManager::GetParticleCSEmitterTexture(summonIndex_).SetEmit(false);
	ParticleManager::GetParticleCSEmitter(dushTrailIndex_).SetEmit(false);

	for (int i = 0; i < 8; i++) {
		ParticleManager::GetSphereEmitter(traceEmitterIndexes_[i]).SetEmit(false);
	}
}

void Boss::Initialize() {
	OriginGameObject::Initialize();
	OriginGameObject::CreateAnimModel("T_boss.gltf");
	animModel_->LoadAnimationFile("T_boss.gltf");

	animModel_->LoadTransformFromJson("boss_transform.json");

	shadow_ = std::make_unique<Object3d>();
	shadow_->Create("Sphere");
	shadow_->SetTexture("white2x2.png");
	shadow_->SetColor({ 0.02f,0.02f,0.02f,0.0f });
	shadow_->SetLightEnable(LightMode::kLightNone);
	shadow_->transform.translate = animModel_->transform.translate;
	shadow_->transform.translate.y = 0.15f;
	shadow_->transform.scale = { 3.0f,0.0f,3.0f };
	shadow_->transform.scale.y = 0.1f;

	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetCollisionStayCallback([this](const ColliderInfo& other) {OnCollisionStay(other); });
	collider_->SetCollisionExitCallback([this](const ColliderInfo& other) {OnCollisionExit(other); });
	collider_->SetParent(&animModel_->transform);
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

	float parentRotate = std::numbers::pi_v<float> *0.25f;
	for (int i = 0; i < 8; i++) {
		std::unique_ptr<Object3d> chargeParent;
		chargeParent = std::make_unique<Object3d>();
		chargeParent->Create("cube.obj");
		chargeParent->transform.translate.y += 20.0f;
		chargeParent->transform.scale.x = params_.beam.baseChargeSize;
		chargeParent->transform.scale.y = params_.beam.baseChargeSize;
		chargeParent->transform.scale.z = params_.beam.baseChargeSize;
		chargeParent->SetParent(&animModel_->transform);
		chargeParent->SetNoneScaleParent(true);
		if (i != 0 && i != 4) {
			if (i < 4) {
				chargeParent->transform.rotate.x = Random::GetFloat(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);
				chargeParent->transform.rotate.y = Random::GetFloat(-1.56f, 1.56f);
			} else {
				chargeParent->transform.rotate.x = Random::GetFloat(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);
				chargeParent->transform.rotate.y = Random::GetFloat(-1.56f, 1.56f);
			}
		}
		chargeParent->transform.rotate.z = parentRotate * i;
		chargeParents_.push_back(std::move(chargeParent));
	}
	for (int i = 0; i < 8; i++) {
		std::unique_ptr<Object3d> anchor;
		anchor = std::make_unique<Object3d>();
		anchor->Create("cube.obj");
		anchor->SetParent(&chargeParents_[i]->transform);
		anchor->SetNoneScaleParent(true);
		traceAnchors_.push_back(std::move(anchor));

		int numEmitter = ParticleManager::GetInstance()->InitGPUEmitter();
		auto& emitter = ParticleManager::GetSphereEmitter(numEmitter);
		emitter.isEmit_ = false;
		emitter.data_->colorMax = { 1.0f,0.0f,0.0f };
		emitter.data_->colorMin = { 1.0f,0.0f,0.0f };
		emitter.data_->frequency = 0.0f;
		emitter.data_->radius = 0.0f;
		emitter.data_->scale = { 1.5f,1.5f,1.5f };
		emitter.data_->lifeTime = 35.0f;
		traceEmitterIndexes_.push_back(numEmitter);
	}

	waveParent_ = std::make_unique<Object3d>();
	waveParent_->Create("cube.obj");
	waveParent_->transform.translate.z += 8.0f;
	waveParent_->SetParent(&animModel_->transform);
	waveParent_->SetNoneScaleParent(true);

	for (int i = 0; i < 4; i++) {
		std::unique_ptr<Object3d> arrowParent;

		arrowParent = std::make_unique<Object3d>();
		arrowParent->Create("cube.obj");

		arrowParent->transform.translate.x = 12.0f - float(i) * 5.0f;
		if (i > 1) {
			arrowParent->transform.translate.x = -12.0f + float(i - 2) * 5.0f;
		}
		arrowParent->transform.translate.y += 6.0f;
		arrowParent->transform.translate.z -= 2.0f;
		arrowParent->SetParent(&animModel_->transform);
		arrowParent->SetNoneScaleParent(true);

		arrowParents_.push_back(std::move(arrowParent));
	}

	ParticleManager::Load(waveAttack1, "ShockRay");
	ParticleManager::Load(waveAttack2, "ShockWaveGround");
	ParticleManager::Load(waveAttack3, "ShockWaveParticle");
	ParticleManager::Load(waveAttack4, "ShockWave");
	ParticleManager::Load(jumpWave_, "JumpShockWave");

	waveAttack1.frequencyTime_ = 0.0f;
	waveAttack2.frequencyTime_ = 0.0f;
	waveAttack3.frequencyTime_ = 0.0f;
	waveAttack4.frequencyTime_ = 0.0f;
	waveAttack1.pos_.z = 4.0f;
	waveAttack2.pos_.z = 4.0f;
	waveAttack3.pos_.z = 4.0f;
	waveAttack4.pos_.z = 4.0f;
	jumpWave_.frequencyTime_ = 0.0f;

	waveAttack1.isAddRandomSize_ = true;
	waveAttack1.addRandomMax_ = { 0.75f,1.2f };
	waveAttack1.addRandomMin_.y = -0.5f;

	waveAttack1.SetParent(&waveParent_->transform);
	waveAttack2.SetParent(&waveParent_->transform);
	waveAttack3.SetParent(&waveParent_->transform);
	waveAttack4.SetParent(&waveParent_->transform);

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

	charge9_.SetParent(&chargeParents_[0]->transform);
	charge10_.SetParent(&chargeParents_[0]->transform);
	charge11_.SetParent(&chargeParents_[0]->transform);
	charge12_.SetParent(&chargeParents_[0]->transform);
	charge13_.SetParent(&chargeParents_[0]->transform);
	charge14_.SetParent(&chargeParents_[0]->transform);
	charge15_.SetParent(&chargeParents_[0]->transform);

	ParticleManager::Load(roringWave_, "roringWave");
	ParticleManager::Load(roringParticle_, "roringParticle");
	ParticleManager::Load(roringring_, "roringring");
	roringWave_.pos_ = animModel_->transform.translate;
	roringWave_.grain_.isAutoUVMove_ = true;
	roringWave_.grain_.autoUVSpeed_.x = 0.01f;
	roringWave_.grain_.isZandX_ = true;
	roringring_.SetAnimParent(animModel_->GetJointTrans("mixamorig:Head"));
	roringring_.pos_.y = -7.0f;
	roringParticle_.SetAnimParent(animModel_->GetJointTrans("mixamorig:Head"));
	roringParticle_.pos_.y = -6.5f;
	roringParticle_.pos_.z = 5.0f;

	ParticleManager::Load(dushStartParticle_, "duahChargeCompleteParticle");
	ParticleManager::Load(dushStartCircle_, "duahChargeCompleteCircle");
	ParticleManager::Load(dushSmoke_, "dushGroundSmoke");
	dushStartParticle_.frequencyTime_ = 0.0f;
	dushStartCircle_.frequencyTime_ = 0.0f;
	dushTrailIndex_ = ParticleManager::GetInstance()->InitGPUEmitter(1);
	ParticleManager::GetParticleCSEmitter(dushTrailIndex_).Load("DushTrail");

	actionList_ = {
		"Root","Wave","Beam","Jump","Sword","Area","Arrow","FallRod","Dush",
	};
	LoadPhase();
	ChangeBehavior(std::make_unique<BossRoot>(this));
	InitSummon();
	animModel_->ChangeAnimation("idle");

	jumpAttackSE_ = &AudioPlayer::GetInstance()->SoundLoadWave("jumpAttackSE.wav");
}

void Boss::Update() {
	if (!isDying_ && isHpActive_ && !isStart_) {
		behavior_->Update();

		core_->Update();

		beam_->Update();
		UpdateWaveWall();
		UpdateArrows();
		UpdateRod();
		UpdateUnderRing();

		ShakeHP();

	} else if (isStart_) { // 開始時の召喚処理
		if (EnergyUpdate()) {
			startTime_ -= FPSKeeper::DeltaTime();
			if (startTime_ > 40.0f && startTime_ < 200.0f) {
				CameraManager::GetInstance()->GetCamera()->IssuanceShake(0.2f, 2.0f);
				roringWave_.Emit();
				roringParticle_.Emit();
				roringring_.Emit();
			}
			if (startTime_ < 0.0f) {
				isStart_ = false;
				ChangeBehavior(std::make_unique<BossRoot>(this));
			}
		}
	} else if (!isHpActive_) { // HPの段階が切り替わった際の処理
		hpCooltime_ -= FPSKeeper::DeltaTime();
		RadialUpdate();

		UpdateWaveWall();
		UpdateArrows();
		UpdateRod();
		UpdateUnderRing();
		if (hpCooltime_ < 0.0f) {
			isHpActive_ = true;
			ChangeBehavior(std::make_unique<BossRoot>(this));
			animModel_->IsRoopAnimation(true);
		}
	} else {
		dyingTime_ -= FPSKeeper::DeltaTime();
		if (dyingTime_ < 0.0f) {
			isClear_ = true;
		}
	}

	animModel_->AnimationUpdate();
	shadow_->transform.translate = animModel_->transform.translate;
	shadow_->transform.translate.y = 0.15f;
	collider_->InfoUpdate();
}

void Boss::Draw([[maybe_unused]] Material* mate, [[maybe_unused]] bool is) {
	shadow_->Draw();
#ifdef _DEBUG
	collider_->DrawCollider();
#endif // _DEBUG

	animModel_->Draw();
	core_->Draw();
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

	for (auto& ring : undderRings_) {
		if (!ring->GetIsLive())continue;
		ring->Draw();
#ifdef _DEBUG
		ring->DrawCollider();
#endif // _DEBUG
	}

	beam_->Draw();

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

void Boss::CSDispatch() {
	animModel_->CSDispatch();
}

void Boss::AnimDraw() {
	animModel_->Draw();
}

void Boss::DebugGUI() {
#ifdef _DEBUG
	if (ImGui::CollapsingHeader("Boss##0")) {
		ImGui::Indent();
		if (ImGui::CollapsingHeader("Boss##1")) {
			animModel_->DebugGUI();
			collider_->DebugGUI();
			ParameterGUI();
		}
		core_->DebugGUI();
		ImGui::Unindent();
	}
#endif // _DEBUG
}

void Boss::ParameterGUI() {
#ifdef _DEBUG
	ImGui::Indent();
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Selected;
	if (ImGui::TreeNodeEx("BossBehavior", flags)) {
		static int currentActionIndex = 0;
		static std::string nowAction = actionList_[currentActionIndex];
		// コンボボックスの表示に使うため、const char* の配列に変換
		std::vector<const char*> actionCStrs;
		for (const auto& str : actionList_) {
			actionCStrs.push_back(str.c_str());
		}

		if (ImGui::Combo("Action", &currentActionIndex, actionCStrs.data(), static_cast<int>(actionCStrs.size()))) {
			nowAction = actionList_[currentActionIndex];
		}
		if (ImGui::Button("SetAction")) {
			SetDefaultBehavior(true);
			static const std::unordered_map<std::string, std::function<std::unique_ptr<BaseBossBehavior>(Boss*)>> behaviorFactory = {
				{ "Root",   [](Boss* b) { return std::make_unique<BossRoot>(b); } },
				{ "Wave", [](Boss* b) { return std::make_unique<BossAttack>(b); } },
				{ "Beam",   [](Boss* b) { return std::make_unique<BossBeamAttack>(b); } },
				{ "Jump",   [](Boss* b) { return std::make_unique<BossJumpAttack>(b); } },
				{ "Sword",   [](Boss* b) { return std::make_unique<BossSwordAttack>(b); } },
				{ "Area",   [](Boss* b) { return std::make_unique<BossAreaAttack>(b); } },
				{ "Arrow",   [](Boss* b) { return std::make_unique<BossArrowAttack>(b); } },
				{ "FallRod",   [](Boss* b) { return std::make_unique<BossRodFall>(b); } },
				{ "Dush",   [](Boss* b) { return std::make_unique<BossDushAttack>(b); } },
				// 他も追加
			};
			auto it = behaviorFactory.find(nowAction);
			if (it != behaviorFactory.end()) {
				ChangeBehavior(it->second(this));
			} else {
				ImGui::Text("Unknown Action: %s", nowAction.c_str());
			}
		}
		if (nowAction != "Root") {
			ImGui::SameLine();
			if (ImGui::Button("AddAction")) {
				phaseList_[phaseIndex_].push_back({ nowAction, 1.0f });
			}
		}


		int phase = int(phaseList_.size());
		if (phase != 0) {
			if (ImGui::BeginTable("table_Phase", phase, ImGuiTableFlags_Borders)) {
				for (int i = 1; i < phase + 1; i++) {
					ImGui::TableSetupColumn(("Phase" + std::to_string(i)).c_str());
				}
				ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
				for (int column = 0; column < phase; column++) {
					ImGui::TableSetColumnIndex(column);
					const char* column_name = ImGui::TableGetColumnName(column); // Retrieve name passed to TableSetupColumn()
					ImGui::PushID(column);
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
					ImGui::RadioButton("##checkall", &phaseIndex_, column);
					ImGui::PopStyleVar();
					ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
					ImGui::TableHeader(column_name);
					ImGui::PopID();
				}

				ImGui::TableNextRow();
				for (int phaseCount = 0; phaseCount < phase; phaseCount++) {
					ImGui::TableSetColumnIndex(phaseCount);
					int tableCount = 2;
					if (phaseList_[phaseCount].size() > 1) tableCount++;
					if (ImGui::BeginTable("table_ActionList", tableCount, ImGuiTableFlags_Borders)) {
						ImGui::TableSetupColumn("Action");
						ImGui::TableSetupColumn("Weight");
						if (tableCount == 3) ImGui::TableSetupColumn("Delete");
						ImGui::TableHeadersRow();

						for (int row = 0; row < phaseList_[phaseCount].size(); row++) {
							ImGui::TableNextRow();
							if (row == 0) {
								ImGui::TableSetColumnIndex(1);
								ImGui::PushItemWidth(50.0f);
								if (tableCount == 3) {
									ImGui::TableSetColumnIndex(2);
									ImGui::PushItemWidth(50.0f);
								}
							}

							// Draw our contents
							ImGui::PushID(row);
							ImGui::TableSetColumnIndex(0);
							ImGui::Text(phaseList_[phaseCount][row].first.c_str());
							ImGui::TableSetColumnIndex(1);
							ImGui::SliderFloat(("##" + phaseList_[phaseCount][row].first + std::to_string(phaseCount)).c_str(), &phaseList_[phaseCount][row].second, 0.0f, 1.0f);
							if (tableCount == 3) {
								ImGui::TableSetColumnIndex(2);
								if (ImGui::Button(("X##del" + phaseList_[phaseCount][row].first + std::to_string(phaseCount)).c_str())) {
									phaseList_[phaseCount].erase(phaseList_[phaseCount].begin() + row);
								}
							}
							ImGui::PopID();
						}
						ImGui::EndTable();
					}
				}

				ImGui::EndTable();
			}
		}
		if (ImGui::Button(("save##phase"))) {
			SavePhase();
		}ImGui::SameLine();
		if (ImGui::Button(("load##phase"))) {
			LoadPhase();
		}

		ImGui::TreePop();
	}
	ImGui::Unindent();
#endif // _DEBUG
}

void Boss::InitParameter() {
	attackCooldown_ = 120.0f;
	bossHp_ = 100.0f;

	params_.beam.parentRotateStep = std::numbers::pi_v<float>*0.25f;

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
		undderRings_.push_back(std::move(ring));
	}

	for (int i = 0; i < 5; i++) {
		std::unique_ptr<Sprite> hpTex;
		hpTex = std::make_unique<Sprite>();
		hpTex->Load("white2x2.png");
		hpTex->SetColor(damageColor1_);
		hpSprites_.push_back(std::move(hpTex));
		hpSprites_[i]->SetPos({ hpStartPos_.x + (hpSize_.x * i) + (hpIndent * i), hpStartPos_.y, 0.0f });
		hpSprites_[i]->SetSize(hpSize_);
	}

	for (int i = 0; i < 2; i++) {
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

void Boss::ReStart() {
	SetDefaultBehavior(true);
	isHpActive_ = true;
	isDying_ = false;
	isStart_ = true;
	//phaseIndex_ = 0;
	startTime_ = 300.0f;
	animModel_->ChangeAnimation("roaring");
	animModel_->LoadTransformFromJson("boss_transform.json");
	animModel_->transform.rotate.y = 3.14f;
	animModel_->IsRoopAnimation(true);
	animModel_->ChangeAnimation("roaring");
}

void Boss::ReduceBossHP(bool isStrong) {
	if (isHpActive_) {
		if (isStrong) {
			bossHp_ -= 4.0f;
		} else {
			bossHp_ -= 2.0f;
		}
		// フェーズやHPの段階を切り替える為の処理
		switch (BossHPState(nowHpIndex_)) {
		case BossHPState::Max:
			HPColorSet(80.0f, 20.0f);
			if (bossHp_ < 80.0f) {
				ChangePhase(80.0f, 1);
				return;
			}
			break;
		case BossHPState::High:
			HPColorSet(60.0f, 20.0f);
			if (bossHp_ < 60.0f) {
				ChangePhase(60.0f, 1);
				phaseIndex_++;
				return;
			}
			break;
		case BossHPState::Half:
			HPColorSet(40.0f, 20.0f);
			if (bossHp_ < 40.0f) {
				ChangePhase(40.0f, 1);
				return;
			}
			break;
		case BossHPState::Low:
			HPColorSet(30.0f, 10.0f);
			if (bossHp_ < 30.0f) {
				ChangePhase(30.0f, 1);
				return;
			}
			break;
		case BossHPState::Empty:
			HPColorSet(0.0f, 30.0f);
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
			animModel_->ChangeAnimation("dying");
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
		shakeTime_ -= FPSKeeper::DeltaTime();
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
		Vector3 dir = pPlayer_->GetWorldPos() - animModel_->transform.translate;
		dir.y = 0.0f; // 水平方向だけに限定
		dir = dir.Normalize();

		// 目標のY軸角度（ラジアン）
		float targetAngle = std::atan2(dir.x, dir.z); // Z前方軸に対する角度

		Vector3 front = GetFrontOffset(Vector3(0.0f, 0.0f, 1.0f) * 0.05f * FPSKeeper::DeltaTime(), targetAngle);
		animModel_->transform.translate += front;

		CaluModelDir();
	}
}

void Boss::CaluModelDir() {
	Vector3 dir = pPlayer_->GetWorldPos() - animModel_->transform.translate;
	dir.y = 0.0f; // 水平方向だけに限定
	dir = dir.Normalize();

	// 目標のY軸角度（ラジアン）
	float targetAngle = std::atan2(dir.x, dir.z); // Z前方軸に対する角度
	// 現在のY軸角度（モデルの回転）
	float currentAngle = animModel_->transform.rotate.y;

	// 角度差を -π〜+π にラップ
	float delta = targetAngle - currentAngle;
	if (delta > std::numbers::pi_v<float>) delta -= 2.0f * std::numbers::pi_v<float>;
	if (delta < -std::numbers::pi_v<float>) delta += 2.0f * std::numbers::pi_v<float>;

	// 角度補間（例えば線形補間）
	float lerpFactor = 0.1f; // 追従の速さ
	float newAngle = currentAngle + delta * lerpFactor;

	animModel_->transform.rotate.y = newAngle;
}

void Boss::DushInit() {
	float parentRotate = std::numbers::pi_v<float> *0.25f;
	for (int i = 0; i < 8; i++) {
		if (i != 0 && i != 4) {
			chargeParents_[i]->transform.rotate.x = Random::GetFloat(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);
			chargeParents_[i]->transform.rotate.y = Random::GetFloat(-std::numbers::pi_v<float>*0.5f, std::numbers::pi_v<float>*0.5f);
		}
		chargeParents_[i]->transform.translate.y = 4.0f;
		chargeParents_[i]->transform.rotate.z = parentRotate * i;
	}
	chargeTime_ = 70.0f;
	chargeSize_ = params_.beam.baseChargeSize;
	charge15_.grain_.startSize_ = { chargeSize_ * 3.0f,chargeSize_ * 6.0f };
	for (auto& chargeParent : chargeParents_) {
		chargeParent->transform.scale = Vector3::FillVec(chargeSize_);
	}

	for (int i = 0; i < 8; i++) {
		auto& emitter = ParticleManager::GetSphereEmitter(traceEmitterIndexes_[i]);
		emitter.isEmit_ = true;
		emitter.data_->lifeTime = 35.0f;
		UpdateEmitterPos(i);
		emitter.data_->prevTranslate = traceAnchors_[i]->GetWorldPos();

		Vector3 randColor = Random::GetVector3({ 0.0f,0.0f }, { 0.2f,0.8f }, { 0.2f,1.0f });
		emitter.data_->colorMax = randColor;
		emitter.data_->colorMin = randColor;
	}
}

bool Boss::DushCharge(float& t, float maxT, bool& isNear, float reng) {
	if (t <= 0.0f) {
		DushInit();
	}
	if (t <= maxT * 0.75f) {
		CaluModelDir();
	}
	t += FPSKeeper::DeltaTime();

	for (int i = 0; i < 8; i++) {
		auto& emitter = ParticleManager::GetSphereEmitter(traceEmitterIndexes_[i]);
		emitter.data_->prevTranslate = traceAnchors_[i]->GetWorldPos();

		if (chargeParents_[i]->transform.scale.x > 0.0f) { // チャージのサイズを縮小していく
			ShrinkScale(i, chargeSize_ / chargeTime_);
			chargeParents_[i]->transform.rotate.z += 0.05f * FPSKeeper::DeltaTime();
		}
		UpdateEmitterPos(i);
	}

	if (Vector3(pPlayer_->GetWorldPos() - animModel_->transform.translate).Length() < reng) {
		isNear = true;
	} else {
		isNear = false;
	}
	if (t > maxT) {
		Vector3 emitPos = GetFrontOffset(Vector3(0.0f, 0.0f, 8.5f), animModel_->transform.rotate.y);
		emitPos += animModel_->transform.translate;
		emitPos.y = 4.0f;
		dushStartParticle_.pos_ = emitPos;
		dushStartCircle_.pos_ = emitPos;
		dushStartParticle_.Emit();
		dushStartCircle_.Emit();
		for (int i = 0; i < 8; i++) {
			ParticleManager::GetSphereEmitter(traceEmitterIndexes_[i]).SetEmit(false);
		}
		return true;
	}
	return false;
}

bool Boss::DushAttack(bool isNear, float& dushReng, float stopReng) {
	if (dushReng == 0.0f) {
		ParticleManager::GetParticleCSEmitter(dushTrailIndex_).SetEmit(true);
		ParticleManager::GetParticleCSEmitter(dushTrailIndex_).SetPos(animModel_->transform.translate);
	}
	isNowDush_ = true;
	Vector3 front = GetFrontOffset(Vector3(0.0f, 0.0f, 2.5f), animModel_->transform.rotate.y);
	animModel_->transform.translate += front;
	dushReng += front.Length();
	Vector3 emitPos = GetFrontOffset(Vector3(0.0f, 0.0f, 8.5f), animModel_->transform.rotate.y);
	emitPos += animModel_->transform.translate;
	emitPos.y = 3.0f;
	ParticleManager::GetParticleCSEmitter(dushTrailIndex_).SetPos(emitPos);
	emitPos = GetFrontOffset(Vector3(0.0f, 0.0f, 5.5f), animModel_->transform.rotate.y);
	emitPos += animModel_->transform.translate;
	emitPos.y = 1.0f;
	dushSmoke_.pos_ = emitPos;
	dushSmoke_.Emit();
	if (!isNear && dushReng >= stopReng * 1.5f) {
		ParticleManager::GetParticleCSEmitter(dushTrailIndex_).SetEmit(false);
		isNowDush_ = false;
		return true;
	} else if (isNear && dushReng >= stopReng) {
		ParticleManager::GetParticleCSEmitter(dushTrailIndex_).SetEmit(false);
		isNowDush_ = false;
		return true;
	}
	return false;
}

void Boss::UpdateWaveWall() {
	for (auto& wall : walls_) {
		if (!wall->GetIsLive())continue;
		wall->CalculetionFollowVec(pPlayer_->GetWorldPos());
		wall->Update();
	}
}

void Boss::WaveWallAttack() {
	int count = 0;

	CameraManager::GetInstance()->GetCamera()->IssuanceShake(0.2f, 15.0f);

	// 波攻撃の出現位置を決める
	Vector3 wavePos = GetFrontOffset(Vector3(0.0f, 0.0f, 4.5f), animModel_->transform.rotate.y);
	wavePos += animModel_->transform.translate;
	wavePos.y = 0.0f;
	for (auto& wall : walls_) {
		if (count == 3) break;
		if (wall->GetIsLive()) continue;
		Vector3 velocity = { 0.0f,0.0f,1.0f };
		if (count == 1) velocity = Vector3(-1.0f, 0.0f, 1.0f).Normalize();
		if (count == 2) velocity = Vector3(1.0f, 0.0f, 1.0f).Normalize();
		velocity = GetFrontOffset(velocity, animModel_->transform.rotate.y);

		wall->InitWave(wavePos, velocity);
		count++;
	}

	waveAttack1.Emit();
	waveAttack2.Emit();
	waveAttack3.Emit();
	waveAttack4.Emit();

}

void Boss::UpdateArrows() {
	for (auto& arrow : arrows_) {
		if (!arrow->GetIsLive())continue;
		arrow->TargetSetting(pPlayer_->GetWorldPos());
		arrow->Update();
	}
}

void Boss::ArrowAttack() {
	int count = 0;

	for (auto& arrow : arrows_) {
		if (count == 4) break;
		if (arrow->GetIsLive()) continue;

		// 矢の位置と飛ぶまでの時間を決める
		Vector3 arrowPos = arrowParents_[count]->GetWorldPos();
		float emittTime = 50.0f;
		if (count == 0) emittTime = 50.0f;
		if (count == 1) emittTime = 80.0f;
		if (count == 2) emittTime = 110.0f;
		if (count == 3) emittTime = 140.0f;
		arrow->InitArrow(arrowPos, emittTime);
		count++;
	}
}

void Boss::UpdateRod() {
	for (auto& rod : rods_) {
		if (rod->GetIsBroke()) RodUnderRing(rod->GetWorldPos());
		if (!rod->GetIsLive())continue;
		rod->RodUpdate();
	}
}

void Boss::RodFall() {
	int count = 0;

	for (auto& rod : rods_) {
		if (count == 6) break;
		if (rod->GetIsLive()) continue;

		// ロッドの出現位置を決める
		Vector3 rodPos = animModel_->transform.translate;
		rodPos.x += Random::GetFloat(-60.0f, 60.0f);
		rodPos.y = 20.0f;
		rodPos.z += Random::GetFloat(-60.0f, 60.0f);
		float emittTime = 140.0f;
		rod->InitRod(rodPos, emittTime);
		count++;
	}
}

void Boss::RodUnderRing(const Vector3& emitPos) {
	int count = 0;
	for (auto& ring : undderRings_) {
		if (count == 1) break;
		if (ring->GetIsLive()) continue;
		ring->InitRing(emitPos, 120.0f);
		count++;
	}
}

void Boss::InitBeam() {
	auto& bp = params_.beam;
	float parentRotate = bp.parentRotateStep;
	for (int i = 0; i < 8; i++) {
		if (i != 0 && i != 4) {
			chargeParents_[i]->transform.rotate.x = Random::GetFloat(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);
			chargeParents_[i]->transform.rotate.y = Random::GetFloat(-std::numbers::pi_v<float>*0.5f, std::numbers::pi_v<float>*0.5f);
		}
		chargeParents_[i]->transform.translate.y = bp.parentY;
		chargeParents_[i]->transform.rotate.z = parentRotate * i;
	}

	chargeTime_ = bp.chargeTime;
	chargeSize_ = bp.baseChargeSize;
	charge15_.grain_.startSize_ = { chargeSize_ * bp.startSizeMulX,chargeSize_ * bp.startSizeMulY };

	for (auto& chargeParent : chargeParents_) {
		chargeParent->transform.scale = Vector3::FillVec(chargeSize_);
	}

	for (int i = 0; i < 8; i++) {
		auto& emitter = ParticleManager::GetSphereEmitter(traceEmitterIndexes_[i]);
		emitter.isEmit_ = true;
		emitter.data_->lifeTime = bp.emitterLifeTime;
		UpdateEmitterPos(i);
		emitter.data_->prevTranslate = traceAnchors_[i]->GetWorldPos();

		Vector3 randColor = Random::GetVector3({ 0.3f,1.0f }, { 0.0f,0.2f }, { 0.0f,0.5f });
		emitter.data_->colorMax = randColor;
		emitter.data_->colorMin = randColor;
	}
}

bool Boss::BeamCharge() {
	auto& bp = params_.beam;
	bool result = false;
	if (ParticleManager::GetParticleCSEmitterSurface(1).IsEmit() == false) {
		auto& emitter = ParticleManager::GetParticleCSEmitterSurface(1);
		emitter.isEmit_ = true;
		emitter.data_->radius = 0.0f;
		emitter.data_->translate = animModel_->transform.translate;
		emitter.data_->translate.y = bp.parentY;
	}

	if (chargeSize_ > 0.0f) {
		ParticleManager::GetParticleCSEmitterSurface(1).data_->radius += bp.radiusGrowSpeed * FPSKeeper::DeltaTime();
		for (int i = 0; i < 8; i++) {
			if (i > 2) {
				if (!(chargeTime_ < bp.chargeTime - i * 2.0f)) {
					continue;
				}
			}

			auto& emitter = ParticleManager::GetSphereEmitter(traceEmitterIndexes_[i]);
			emitter.data_->prevTranslate = traceAnchors_[i]->GetWorldPos();

			if (chargeParents_[i]->transform.scale.x > 0.0f) { // チャージのサイズを縮小していく
				ShrinkScale(i, bp.shrinkSpeed * FPSKeeper::DeltaTime());
				chargeParents_[i]->transform.rotate.z += bp.rotateSpeedZ * FPSKeeper::DeltaTime();
			}
			if (chargeParents_[i]->transform.scale.x <= 0.0f) {
				if (i == 0) {
					chargeSize_ -= bp.sizeDecrease;
					if (chargeSize_ <= 0.0f) {
						chargeSize_ = 0.0f;
					}
				}
				chargeParents_[i]->transform.scale = Vector3::FillVec(chargeSize_);
				float emitpos = chargeParents_[i]->transform.scale.x;
				traceAnchors_[i]->transform.translate = { emitpos,emitpos,emitpos };

				emitter.data_->prevTranslate = traceAnchors_[i]->GetWorldPos();
				emitter.data_->lifeTime -= bp.lifeTimeDecrease;
			}
			UpdateEmitterPos(i);
		}

		if (chargeSize_ > bp.minRecalcSize) {
			charge15_.grain_.startSize_ = { chargeSize_ * bp.startSizeMulX,chargeSize_ * bp.startSizeMulY };
		}

		charge9_.Emit();
		charge10_.Emit();
		charge11_.Emit();
		charge15_.Emit();
		chargeTime_ -= FPSKeeper::DeltaTime();
	} else if (chargeSize_ <= 0.0f) {
		result = true;
		for (int i = 0; i < 8; i++) {
			if (chargeParents_[i]->transform.scale.x > 0.0f) { // 完全に真ん中に集結するようにする
				auto& emitter = ParticleManager::GetSphereEmitter(traceEmitterIndexes_[i]);
				emitter.data_->prevTranslate = traceAnchors_[i]->GetWorldPos();

				ShrinkScale(i, bp.shrinkSpeed * FPSKeeper::DeltaTime());
				chargeParents_[i]->transform.rotate.z += bp.rotateSpeedZ * FPSKeeper::DeltaTime();
				result = false;
			}
			if (!result) {
				UpdateEmitterPos(i);
			}
		}
	}

	if (result) {
		chargeParents_[0]->transform.scale = { 1.0f,1.0f,1.0f };
	}

	return result;
}

void Boss::BeamChargeComplete() {
	for (int i = 0; i < 8; i++) {
		ParticleManager::GetSphereEmitter(traceEmitterIndexes_[i]).SetEmit(false);
	}
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

void Boss::InitJumpAttack() {
	jumpTime_ = params_.jump.jumpTotalTime;
	isJumpAttack_ = true;
}

bool Boss::JumpAttack() {

	if (jumpTime_ > 0.0f) {
		jumpTime_ -= FPSKeeper::DeltaTime();
		if (jumpTime_ < 0.0f) {
			jumpTime_ = 0.0f;
		}
	}
	auto& jp = params_.jump;

	// 時間でジャンプの挙動を制御する
	if (jumpTime_ <= jp.upStart && jumpTime_ >= jp.upEnd) {//120~90 //30

		float upTime = jp.upStart - jp.upEnd;
		float flyT = 1.0f - ((jumpTime_ - jp.upEnd) / upTime);
		animModel_->transform.translate.y = std::lerp(0.0f, jp.height, (1.0f - (1.0f - flyT) * (1.0f - flyT)));
	} else if (jumpTime_ < jp.downStart && jumpTime_ >= jp.downEnd) {//70~50 //20

		float downTime = jp.downStart - jp.downEnd;
		float flyT = 1.0f - (jumpTime_ - jp.downEnd) / downTime;
		animModel_->transform.translate.y = std::lerp(jp.height, 0.0f, (1.0f - powf(1.0f - flyT, 4.0f)));
		if (std::abs(animModel_->transform.translate.y) < jp.groundJudgeHeight) {
			if (isJumpAttack_) {
				jumpWave_.pos_ = animModel_->transform.translate;
				jumpWave_.Emit();
				AudioPlayer::GetInstance()->SoundPlayWave(*jumpAttackSE_, 0.6f);
				UnderRingEmit();
			}
		}
	} else if (jumpTime_ <= 0.0f) {
		animModel_->transform.translate.y = 0.0f;
		return true;
	}

	return false;
}

void Boss::UpdateUnderRing() {
	for (auto& ring : undderRings_) {
		if (!ring->GetIsLive())continue;
		ring->Update();
	}
}

void Boss::UnderRingEmit() {
	CameraManager::GetInstance()->GetCamera()->IssuanceShake(0.3f, 30.0f);
	isJumpAttack_ = false;
	int count = 0;
	for (auto& ring : undderRings_) {
		if (count == 1) break;
		if (ring->GetIsLive()) continue;
		ring->InitRing(animModel_->transform.translate);
		count++;
	}
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

float Boss::GetChainRate() {
	return chainRate_ + (chainCount_ - 1) * 0.05f;
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
	for (int i = 0; i < 8; i++) {
		ParticleManager::GetSphereEmitter(traceEmitterIndexes_[i]).SetEmit(false);
	}
	ParticleManager::GetParticleCSEmitter(dushTrailIndex_).SetEmit(false);
	if (isInvisibleItem) {
		for (auto& wave : walls_) {
			wave->SetIsLive(false);
		}
		for (auto& arrow : arrows_) {
			arrow->SetIsLive(false);
		}
		for (auto& rod : rods_) {
			rod->SetIsLive(false);
		}
		for (auto& ring : undderRings_) {
			ring->SetIsLive(false);
		}
	}
	cameraRang_ = -25.0f;
	cameraFollowSpeed_ = 0.2f;
	ChangeBehavior(std::make_unique<BossRoot>(this));
	animModel_->transform.translate.y = 0.0f;
	Vector3 dir = pPlayer_->GetWorldPos() - animModel_->transform.translate;
	dir.y = 0.0f; // 水平方向だけに限定
	dir = dir.Normalize();
	// 目標のY軸角度（ラジアン）
	float targetAngle = std::atan2(dir.x, dir.z); // Z前方軸に対する角度
	animModel_->transform.translate += GetFrontOffset(Vector3(0.0f, 0.0f, 1.0f) * 0.05f * FPSKeeper::DeltaTime(), targetAngle);
	// 現在のY軸角度（モデルの回転）
	float currentAngle = animModel_->transform.rotate.y;
	// 角度差を -π〜+π にラップ
	float delta = targetAngle - currentAngle;
	if (delta > std::numbers::pi_v<float>) delta -= 2.0f * std::numbers::pi_v<float>;
	if (delta < -std::numbers::pi_v<float>) delta += 2.0f * std::numbers::pi_v<float>;
	float newAngle = currentAngle + delta;
	animModel_->transform.rotate.y = newAngle;
}

void Boss::RadialSetting() {
	radialtime_ = params_.radial.baseTime;
	dxcommon_->GetOffscreenManager()->AddPostEffect(PostEffectList::Radial);
	dxcommon_->GetOffscreenManager()->SetRadialParamsWidth(params_.radial.widthStart);
}

void Boss::RadialUpdate() {
	if (radialtime_ > 0.0f) {
		radialtime_ -= FPSKeeper::DeltaTime();
		if (radialtime_ <= 0.0f) {
			radialtime_ = 0.0f;
			dxcommon_->GetOffscreenManager()->PopPostEffect(PostEffectList::Radial);
		}
		auto& rp = params_.radial;
		float t = radialtime_ / rp.baseTime;
		float radialwidth = std::lerp(rp.widthStart, rp.widthEnd, 1.0f - powf(1.0f - t, 3.0f));
		dxcommon_->GetOffscreenManager()->SetRadialParamsWidth(radialwidth);
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

void Boss::InitSummon() {
	summonIndex_ = ParticleManager::GetInstance()->InitGPUEmitterTexture("magicCircle.png");

	auto& emitter = ParticleManager::GetParticleCSEmitterTexture(summonIndex_);
	emitter.SetEmit(true);
	emitter.data_->lifeTime = 50.0f;
	emitter.data_->radius = 10.0f;
	emitter.data_->frequency = 1.0f;
	emitter.data_->translate = animModel_->transform.translate;
	emitter.data_->baseVelocity = { 0.0f,0.01f,0.0f };

	bossYPos_ = animModel_->transform.translate.y;
	defaultCorePos_ = core_->GetWorldPos();
	animModel_->transform.translate.y = -30.0f;

	ParticleManager::Load(summonLightning_, "summonLightning_");
	ParticleManager::Load(energySphere_, "energySphere");
	ParticleManager::Load(energyParticle_, "energyParticle");

	summonLightning_.grain_.isZandX_ = true;
	energySphere_.grain_.isZandX_ = true;

	summonLightning_.pos_ = animModel_->transform.translate;
	summonLightning_.pos_.y = 40.0f;
	energySphere_.pos_ = animModel_->transform.translate;
	energySphere_.pos_.y = 0.0f;
	energyParticle_.pos_ = animModel_->transform.translate;
	energyParticle_.pos_.y = 0.0f;

	energySphere_.grain_.isAutoUVMove_ = true;
	energySphere_.grain_.autoUVSpeed_ = { 0.01f,0.01f };
	energySphere_.para_.autoUVMax = { 0.02f,0.01f };
	energySphere_.para_.autoUVMin = { -0.02f,-0.01f };

}

void Boss::ExpandSummon() {
	if (FPSKeeper::DeltaTime() > FPSKeeper::GetClampFrame()) return;
	if (startWaiting_ > 0.0f) {
		startWaiting_ -= FPSKeeper::DeltaTime();
		return;
	}
	if (summonCircleExpandTime_ > 0.0f) { // 召喚陣の拡大
		isSummon_ = true;
		auto& sp = params_.summon;

		summonCircleExpandTime_ -= FPSKeeper::DeltaTime();
		float t = (std::max)(summonCircleExpandTime_ / sp.summonExpandTime, 0.0f);
		float summonRadius = std::lerp(sp.summonRadiusStart, sp.summonRadiusEnd, t);
		ParticleManager::GetParticleCSEmitterTexture(summonIndex_).data_->radius = summonRadius;
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
		energyTime_ -= FPSKeeper::DeltaTime();
		if (energyTime_ >= sp.bossRiseStartTime) { // ボスの位置更新
			float t = (std::max)((energyTime_ - sp.bossRiseStartTime) / sp.bossRiseStartTime, 0.0f);
			animModel_->transform.translate.y = std::lerp(bossYPos_, sp.bossDownPos, t);
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
		energyCoolTime_ -= FPSKeeper::DeltaTime();
		if (energyCoolTime_ <= 0.0f) {
			isSummon_ = false;
			animModel_->transform.translate.y = bossYPos_;
			animModel_->ChangeAnimation("roaring");
			ParticleManager::GetParticleCSEmitterTexture(summonIndex_).SetEmit(false);
			shadow_->SetColor({ 0.02f,0.02f,0.02f,0.5f });
		}
		summonLightning_.particleRotate_.x = Random::GetFloat(-0.15f, 0.15f);
		summonLightning_.particleRotate_.y = Random::GetFloat(-3.14f, 3.14f);
		summonLightning_.particleRotate_.z = Random::GetFloat(-0.15f, 0.15f);
		summonLightning_.Emit();
	}
}

void Boss::UpdateEmitterPos(int i) {
	float emitpos = chargeParents_[i]->transform.scale.x;
	traceAnchors_[i]->transform.translate = { emitpos, emitpos, emitpos };
	auto& emitter = ParticleManager::GetSphereEmitter(traceEmitterIndexes_[i]);
	emitter.data_->translate = traceAnchors_[i]->GetWorldPos();
}

void Boss::ShrinkScale(int i, float delta) {
	auto& s = chargeParents_[i]->transform.scale;
	s.x = (std::max)(0.0f, s.x - delta);
	s.y = (std::max)(0.0f, s.y - delta);
	s.z = (std::max)(0.0f, s.z - delta);
}

void Boss::ChangePhase(float threshold, int indexInc) {
	bossHp_ = threshold;
	nowHpIndex_ -= indexInc;
	isHpActive_ = false;
	SetDefaultBehavior();
	animModel_->ChangeAnimation("hit");
	animModel_->IsRoopAnimation(false);
	RadialSetting();
	hpCooltime_ = 60.0f;
	hpSprites_[nowHpIndex_]->SetColor(damageColor1_);
}

Vector3 Boss::GetFrontOffset(const Vector3& distance, float angle) {
	Vector3 front = distance;
	Matrix4x4 rot = MakeRotateYMatrix(angle);
	return TransformNormal(front, rot);
}
