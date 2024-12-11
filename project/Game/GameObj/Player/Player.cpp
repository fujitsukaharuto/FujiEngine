#include "Player.h"
#include "Game/GameObj/Player/PlayerDefaultBehavior.h"
#include "Input/Input.h"

Player::Player() {
}

Player::~Player() {
}

void Player::Initialize() {
	OriginGameObject::Initialize();
	model_->Create("suzanne.obj");
	model_->transform.scale = { 0.5f,0.5f,0.5f };


	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetTag("player");


	colliderAttack_ = std::make_unique<AABBCollider>();
	colliderAttack_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionAttackEnter(other); });
	colliderAttack_->SetTag("attack");

	state_ = std::make_unique<PlayerDefaultBehavior>(this);
	behaviorRequest_ = PlayerBehavior::kDefult;
	BehaviorRequest();


}

void Player::Update() {

	BehaviorRequest();

	state_->Update();

	XINPUT_STATE pad;
	if (Input::GetInstance()->GetGamepadState(pad)) {
		if (Input::GetInstance()->TriggerButton(PadInput::B)) {
			if (!isAttack_) {
				isAttack_ = true;
				attackT_ = 30.0f;
				colliderAttack_->SetWidth(1.0f);
				colliderAttack_->SetHeight(1.0f);
				colliderAttack_->SetDepth(1.0f);
			}
			else {
				attackT_ = 0.0f;
				attackT2_ = 20.0f;
				isAttack2_ = true;
				colliderAttack_->SetWidth(3.0f);
				colliderAttack_->SetHeight(1.0f);
				colliderAttack_->SetDepth(3.0f);
			}
		}
	}
	if (isAttack_) {
		attackT_ -= FPSKeeper::DeltaTime();
		
		if (attackT_ <= 0.0f) {
			if (attackT2_ <= 0.0f) {
				isAttack_ = false;
			}
		}
		else {
			const float kCharacterSpeed = 0.3f;
			Vector3 move = { 0.0f, 0.0f, 1.0f };
			move = move.Normalize() * kCharacterSpeed;
			Matrix4x4 rotatePlayer = MakeRotateXYZMatrix(model_->transform.rotate);
			move = TransformNormal(move, rotatePlayer);
			model_->transform.translate += move * FPSKeeper::DeltaTime();
		}
		
		if (isAttack2_) {
			if (attackT2_ <= 0.0f) {
				isAttack_ = false;
				isAttack2_ = false;
			}
			else {
				attackT2_ -= FPSKeeper::DeltaTime();

				float tt = attackT2_ / 20.0f;
				Vector3 pPos = model_->transform.translate;
				model_->transform.translate = Lerp({ pPos.x,0.0f,pPos.z }, { pPos.x,4.0f,pPos.z }, tt * tt);

			}
		}

	}

	collider_->SetPos(model_->GetWorldPos());
	
	Vector3 attackCollider = { 0.0f,0.0f,1.0f };
	Matrix4x4 roteM = MakeRotateYMatrix(model_->transform.rotate.y);
	attackCollider = Transform(attackCollider, roteM);
	attackCollider = attackCollider + model_->transform.translate;
	colliderAttack_->SetPos(attackCollider);
	if (isAttack2_) {
		colliderAttack_->SetPos(model_->GetWorldPos());
	}

}

void Player::Draw([[maybe_unused]]Material* mate) {
	OriginGameObject::Draw(mate);
}

void Player::BehaviorRequest() {
	if (behaviorRequest_) {
		behavior_ = behaviorRequest_.value();

		switch (behavior_) {
		case Player::PlayerBehavior::kDefult:
			SetState(std::make_unique<PlayerDefaultBehavior>(this));
			break;
		case Player::PlayerBehavior::kAttack:
			//SetState(std::make_unique<PlayerDefaultBehavior>(this));
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

#ifdef _DEBUG
void Player::Debug() {

}
#endif // _DEBUG


