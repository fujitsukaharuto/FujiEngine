#include "Player.h"
#include "Game/GameObj/Player/PlayerDefaultBehavior.h"
#include "Game/GameObj/Player/PlayerAttackBehavior.h"
#include "Game/GameObj/Player/PlayerDushBehavior.h"
#include "Game/GameObj/Player/PlayerJumpBehavior.h"
#include "Input/Input.h"
#include "Game/GameObj/LockOn.h"
#include "Model/PlaneDrawer.h"

Player::Player() {
}

Player::~Player() {
	Audio::GetInstance()->SoundStopWave(attackSound1);
	Audio::GetInstance()->SoundStopWave(attackSound2);
	Audio::GetInstance()->SoundStopWave(attackSound3);

	Audio::GetInstance()->SoundStopWave(jumpSound);
	Audio::GetInstance()->SoundStopWave(dushSound);

	PlaneDrawer::GetInstance()->Reset();
}

void Player::Initialize() {
	OriginGameObject::Initialize();
	model_->Create("player.obj");

	body_ = std::make_unique<Object3d>();
	body_->Create("player.obj");
	body_->SetParent(model_.get());
	body_->transform.scale = { 0.5f,0.5f,0.5f };

	shadow_->transform.scale = { 0.7f,0.0f,0.7f };

	weapon_ = std::make_unique<Object3d>();
	weapon_->Create("playerWeapon.obj");
	weapon_->SetParent(model_.get());
	weapon_->transform.scale = { 0.6f,0.8f,0.6f };
	weapon_->transform.translate = { 1.0f,0.5f,0.0f };

	firePlane_ = std::make_unique<Object3d>();
	firePlane_->Create("firePlane.obj");
	firePlane_->SetLightEnable(LightMode::kLightNone);
	firePlane_->SetParent(weapon_.get());
	firePlane_->transform.translate = { 0.0f,0.45f,0.0f };
	firePlane_->transform.scale.y = 1.13f;
	firePlane_->transform.rotate = { 0.0f,1.48f,0.0f };


	slash_ = std::make_unique<SlashDrawer>();
	slash_->Initialize();
	slash_->SetParent(model_.get());
	slash_->GetModel()->transform.translate.y = 1.4f;
	slash_->GetModel()->transform.translate.z = 1.0f;
	slash_->GetModel()->transform.scale.y = 1.4f;
	slash_->GetModel()->transform.scale.z = 1.2f;
	slash_->GetModel()->transform.rotate.z = 1.4f;
	slash_->GetModel()->transform.rotate.y = 1.4f;

	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetTag("player");



	colliderAttack_ = std::make_unique<AABBCollider>();
	colliderAttack_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionAttackEnter(other); });
	colliderAttack_->SetTag("attack");
	colliderAttack_->SetParent(model_.get());

	state_ = std::make_unique<PlayerDefaultBehavior>(this);
	behaviorRequest_ = PlayerBehavior::kDefult;
	BehaviorRequest();


	walkParticle_.name = "walkParticle";
	walkParticle_.Load("walkParticle");
	walkParticle_.SetParent(model_.get());


	attackParticle_.name = "attackParticle";
	attackParticle_.Load("attackParticle");
	attackParticle_.SetParent(weapon_.get());

	attackParticle2_.name = "attackParticle2";
	attackParticle2_.Load("attackParticle2");
	attackParticle2_.SetParent(weapon_.get());


	attackParticle3_.name = "attackParticle3";
	attackParticle3_.Load("attackParticle3");
	attackParticle3_.SetParent(body_.get());

	attackParticle4_.name = "attackParticle4";
	attackParticle4_.Load("attackParticle4");
	attackParticle4_.SetParent(body_.get());

	attackParticle5_.name = "attackParticle5";
	attackParticle5_.Load("attackParticle5");
	attackParticle5_.SetParent(body_.get());

	dushEmit1_.name = "flyEnemy1";
	dushEmit1_.Load("flyEnemy1");
	dushEmit1_.SetParent(body_.get());
	dushEmit2_.name = "flyEnemy2";
	dushEmit2_.Load("flyEnemy2");
	dushEmit2_.SetParent(body_.get());


	attackSound1=Audio::GetInstance()->SoundLoadWave("attack2.wav");
	attackSound2 = Audio::GetInstance()->SoundLoadWave("attack1.wav");
	attackSound3 = Audio::GetInstance()->SoundLoadWave("jumpAttack.wav");

	dushSound = Audio::GetInstance()->SoundLoadWave("dush.wav");
	jumpSound = Audio::GetInstance()->SoundLoadWave("jump01.wav");

}

void Player::Update() {

	BehaviorRequest();

	state_->Update();


	XINPUT_STATE pad;
	if (Input::GetInstance()->GetGamepadState(pad)) {
		if (Input::GetInstance()->TriggerButton(PadInput::X)) {
			if (!isAttack_ && behavior_ == PlayerBehavior::kDefult) {
				behaviorRequest_ = PlayerBehavior::kAttack;
				Vector3 attackCollider = { 0.0f,0.0f,1.0f };
				colliderAttack_->SetPos(attackCollider);
				slash_->SetTimeLimitte(12.0f);
				slash_->ReSet();
			}
			else {
				isAttack2_ = true;
			}
		}
		if (Input::GetInstance()->TriggerButton(PadInput::A)) {
			if (!isAttack_ && behavior_ == PlayerBehavior::kDefult) {
				behaviorRequest_ = PlayerBehavior::kDush;
			}
		}
		if (Input::GetInstance()->TriggerButton(PadInput::B)) {
			if (!isAttack_ && behavior_ == PlayerBehavior::kDefult) {
				behaviorRequest_ = PlayerBehavior::kJump;
			}
		}
	}

#ifdef _DEBUG

	walkParticle_.DebugGUI();

	attackParticle_.DebugGUI();
	attackParticle2_.DebugGUI();

	attackParticle3_.DebugGUI();
	attackParticle4_.DebugGUI();
	attackParticle5_.DebugGUI();

	dushEmit1_.DebugGUI();
	dushEmit2_.DebugGUI();

	ImGui::Begin("weapon");
	ImGui::DragFloat3("rotate", &weapon_->transform.rotate.x, 0.01f);

	ImGui::DragFloat3("rotateSlash", &slash_->GetModel()->transform.rotate.x, 0.01f);
	ImGui::End();
	ImGui::Begin("body");
	ImGui::DragFloat3("rotateBody", &body_->transform.rotate.x, 0.01f);
	ImGui::End();
	ImGui::Begin("fire");
	ImGui::DragFloat3("trans", &firePlane_->transform.translate.x, 0.01f);
	ImGui::DragFloat3("rotate", &firePlane_->transform.rotate.x, 0.01f);
	ImGui::DragFloat3("scake", &firePlane_->transform.scale.x, 0.01f);
	ImGui::End();

#endif // _DEBUG


	collider_->SetPos(model_->GetWorldPos());
	collider_->InfoUpdate();
	colliderAttack_->InfoUpdate();

	slash_->Update();

	if (isAttack_) {
		attackParticle_.Emit();
		attackParticle2_.Emit();

		PlaneDrawer::GetInstance()->AddPlanePoint(attackParticle_.GetWorldPos());
		PlaneDrawer::GetInstance()->AddPlanePoint(attackParticle2_.GetWorldPos());

		slashTime_ = 10.0f;
	}
	else {
		if (firePlane_->transform.scale.x > 0.0f) {
			firePlane_->transform.scale.x -= 0.04f;
			if (firePlane_->transform.scale.x < 0.0f) {
				firePlane_->transform.scale.x = 0.0f;
			}
		}

		if (slashTime_ > 0.0f) {
			slashTime_ -= FPSKeeper::DeltaTime();
			if (slashTime_ <= 0.0f) {
				slashTime_ = 0.0f;
			}
		}
	}
	if (popTime_ > 0.0f) {
		popTime_ -= FPSKeeper::DeltaTime();
	}
	else {
		PlaneDrawer::GetInstance()->PopPlanePoint();
		PlaneDrawer::GetInstance()->PopPlanePoint();
		popTime_ = 1.5f;
	}

	if (firePlane_->transform.scale.x <= 1.0f) {
		firePlane_->SetUVScale({ 1.0f,firePlane_->transform.scale.x }, { 0.0f,0.0f });
	}
	else {
		firePlane_->SetUVScale({ 1.0f,1.0f }, { 0.0f,0.0f });
	}

	model_->UpdateWVP();


}

void Player::Draw([[maybe_unused]]Material* mate) {
	//OriginGameObject::Draw(mate);
	OriginGameObject::ShdowDraw();
	body_->Draw(mate);
	weapon_->Draw(mate);
	//firePlane_->ShaderTextureDraw();
#ifdef _DEBUG
	attackParticle_.DrawSize();
	attackParticle2_.DrawSize();
#endif // _DEBUG

}

void Player::SlashDraw() {
	if (slashTime_ > 0.0f && behavior_ != PlayerBehavior::kJump) {
		slash_->Draw();
	}
}

void Player::BehaviorRequest() {
	if (behaviorRequest_) {
		behavior_ = behaviorRequest_.value();

		switch (behavior_) {
		case Player::PlayerBehavior::kDefult:
			SetState(std::make_unique<PlayerDefaultBehavior>(this));
			break;
		case Player::PlayerBehavior::kAttack:
			SetState(std::make_unique<PlayerAttackBehavior>(this));
			break;
		case Player::PlayerBehavior::kDush:
			SetState(std::make_unique<PlayerDushBehavior>(this));
			break;
		case Player::PlayerBehavior::kJump:
			SetState(std::make_unique<PlayerJumpBehavior>(this));
			break;
		default:
			break;
		}

		behaviorRequest_ = std::nullopt;
	}
}

void Player::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {

}

void Player::OnCollisionAttackEnter([[maybe_unused]] const ColliderInfo& other) {
}

void Player::EmitJumpAttack() {
	attackParticle3_.Burst();
	attackParticle4_.Burst();
	attackParticle5_.Burst();
}

void Player::EmitWalk() {
	walkParticle_.Burst();
}

void Player::EmitDush() {
	dushEmit1_.Emit();
	dushEmit2_.Emit();
}

bool Player::GetLockOn() {
	return lockOn_ && lockOn_->ExistTarget();
}
#ifdef _DEBUG
void Player::Debug() {

}
#endif // _DEBUG


