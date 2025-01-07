#include "PlayerDushBehavior.h"
#include "Game/GameObj/Player/Player.h"
#include "Input/Input.h"
#include "Math/MatrixCalculation.h"
#include "Camera/CameraManager.h"


PlayerDushBehavior::PlayerDushBehavior(Player* pPlayer) :pPlayer_(pPlayer) {
	Audio::GetInstance()->SoundPlayWave(pPlayer_->dushSound);
}

PlayerDushBehavior::~PlayerDushBehavior() {
}

void PlayerDushBehavior::Initialize() {
}

void PlayerDushBehavior::Update() {
	Direction();
	Dush();
}

void PlayerDushBehavior::Dush() {
	attackT1_ -= FPSKeeper::DeltaTime();

	const float kCharacterSpeed = 0.4f;
	Vector3 move = { 0.0f, 0.0f, 1.0f };
	move = move.Normalize() * kCharacterSpeed;
	Matrix4x4 rotatePlayer = MakeRotateXYZMatrix(pPlayer_->GetModel()->transform.rotate);
	move = TransformNormal(move, rotatePlayer);
	pPlayer_->GetModel()->transform.translate += move * FPSKeeper::DeltaTime();

	float t = CustomEasing(1.0f / 30.0f * (30.0f - attackT1_));
	pPlayer_->GetBodyModel()->transform.scale.z = Lerp(0.5f, 0.75f, t);
	float xySize = Lerp(0.5f, 0.25f, t);
	pPlayer_->GetBodyModel()->transform.scale.x = xySize;
	pPlayer_->GetBodyModel()->transform.scale.y = xySize;

	if (attackT1_ <= 0.0f) {
		pPlayer_->SetBehaviorRequest(Player::PlayerBehavior::kDefult);
		EndInit();
	}
}

void PlayerDushBehavior::Direction() {
	XINPUT_STATE pad;
	if (Input::GetInstance()->GetGamepadState(pad)) {

		const float threshold = 0.7f;
		bool ismoving = false;

		Vector2 padStickL = Input::GetInstance()->GetLStick();
		Vector3 velo = { padStickL.x / SHRT_MAX, 0.0f, padStickL.y / SHRT_MAX };

		if (velo.Length() > threshold) {
			ismoving = true;
		}


		if (ismoving) {
			velo = velo.Normalize();

			Vector3 cameraR = CameraManager::GetInstance()->GetCamera()->transform.rotate;
			Matrix4x4 rotateCamera = MakeRotateXYZMatrix(cameraR);
			velo = TransformNormal(velo, rotateCamera);


			float targetRotate = std::atan2(velo.x, velo.z);
			pPlayer_->GetTrans().rotate.y = LerpShortAngle(pPlayer_->GetTrans().rotate.y, targetRotate, 0.25f);

		}

	}
}

void PlayerDushBehavior::EndInit() {
	pPlayer_->GetBodyModel()->transform.scale = { 0.5f,0.5f,0.5f };
}

float PlayerDushBehavior::CustomEasing(float t) {
	if (t <= 0.2) {
		// t = 0 で 0, t = 0.2 で 1 に線形補間
		return t / 0.2f;
	}
	else if (t <= 0.7) {
		// t = 0.2 から t = 0.7 までは 1 を維持
		return 1.0f;
	}
	else {
		// t = 0.7 で 1, t = 1 で 0 に線形補間
		return (1.0f - (t - 0.7f) / 0.3f);
	}
}


//isAttack_ = pPlayer_->GetIsAttack();
//
//XINPUT_STATE pad;
//if (Input::GetInstance()->GetGamepadState(pad)) {
//	if (Input::GetInstance()->TriggerButton(PadInput::B)) {
//		if (!isChain_) {
//			switch (comboIndex_) {
//			case 0:
//				attackT_ = 0.0f;
//				attackT2_ = 20.0f;
//				isAttack2_ = true;
//				pPlayer_->GetAABBAttack()->SetWidth(3.0f);
//				pPlayer_->GetAABBAttack()->SetHeight(1.0f);
//				pPlayer_->GetAABBAttack()->SetDepth(3.0f);
//				break;
//			case 1:
//				break;
//			case 2:
//				break;
//			case 3:
//				break;
//			default:
//				break;
//			}
//		}
//	}
//}
//
//if (isAttack_) {
//	attackT_ -= FPSKeeper::DeltaTime();
//
//	if (attackT_ <= 0.0f) {
//		if (attackT2_ <= 0.0f) {
//			isAttack_ = false;
//			pPlayer_->SetIsAttack(false);
//			pPlayer_->SetBehaviorRequest(Player::PlayerBehavior::kDefult);
//		}
//	}
//	else {
//		const float kCharacterSpeed = 0.3f;
//		Vector3 move = { 0.0f, 0.0f, 1.0f };
//		move = move.Normalize() * kCharacterSpeed;
//		Matrix4x4 rotatePlayer = MakeRotateXYZMatrix(pPlayer_->GetModel()->transform.rotate);
//		move = TransformNormal(move, rotatePlayer);
//		pPlayer_->GetModel()->transform.translate += move * FPSKeeper::DeltaTime();
//	}
//
//	if (isAttack2_) {
//		if (attackT2_ <= 0.0f) {
//			isAttack_ = false;
//			pPlayer_->SetIsAttack(false);
//			pPlayer_->SetBehaviorRequest(Player::PlayerBehavior::kDefult);
//			isAttack2_ = false;
//		}
//		else {
//			attackT2_ -= FPSKeeper::DeltaTime();
//
//			float tt = attackT2_ / 20.0f;
//			Vector3 pPos = pPlayer_->GetModel()->transform.translate;
//			pPlayer_->GetModel()->transform.translate = Lerp({ pPos.x,0.0f,pPos.z }, { pPos.x,4.0f,pPos.z }, tt * tt);
//
//		}
//	}
//
//}
//
//
//Vector3 attackCollider = { 0.0f,0.0f,1.0f };
//Matrix4x4 roteM = MakeRotateYMatrix(pPlayer_->GetModel()->transform.rotate.y);
//attackCollider = Transform(attackCollider, roteM);
//attackCollider = attackCollider + pPlayer_->GetModel()->transform.translate;
//pPlayer_->GetAABBAttack()->SetPos(attackCollider);
//if (isAttack2_) {
//	pPlayer_->GetAABBAttack()->SetPos(pPlayer_->GetModel()->GetWorldPos());
//}