#include "PlayerAttackBehavior.h"
#include "Game/GameObj/Player/Player.h"
#include "Input/Input.h"
#include "Math/MatrixCalculation.h"
#include "Camera/CameraManager.h"


// やらなければならないこと
//
// 1obbの当たり判定
// 2ヒットエフェク
// 3攻撃モーション
// 4雷エフェクト
// 5
// 6
// 1月4日
// 雷エフェクト(後回し)
// 攻撃の動き(薙刀とプレイヤー)
// 
// 




PlayerAttackBehavior::PlayerAttackBehavior(Player* pPlayer) : pPlayer_(pPlayer) {
	pPlayer_->GetAABBAttack()->SetWidth(0.5f);
	pPlayer_->GetAABBAttack()->SetHeight(0.5f);
	pPlayer_->GetAABBAttack()->SetDepth(0.5f);

	isAttack_ = true;
	pPlayer_->SetIsAttack(true);

	attackPoint1_ = {
		{0.0f,1.5f,0.75f},
		{0.0f,0.9f,1.0f},
		{0.0f,0.5f,1.25f},
		{0.0f,0.2f,1.2f},
	};
	attackPoint2_ = {
		{-1.0f,0.5f,0.0f},
		{-0.5f,0.5f,0.5f},
		{0.5f,0.5f,0.5f},
		{1.0f,0.5f,0.0f},
	};
	attackPoint3_ = {
		{-1.0f,1.5f,0.2f},
		{-0.5f,0.9f,0.7f},
		{0.5f,0.5f,1.0f},
		{1.0f,0.2f,1.0f},
	};
	attackPoint4_ = {
		{1.0f,1.5f,0.2f},
		{0.5f,0.9f,0.7f},
		{-0.5f,0.5f,1.0f},
		{-1.0f,0.2f,1.0f},
	};

}

PlayerAttackBehavior::~PlayerAttackBehavior() {
}

void PlayerAttackBehavior::Initialize() {
}

void PlayerAttackBehavior::Update() {
	Attack();
}

void PlayerAttackBehavior::Attack() {

	XINPUT_STATE pad;
	if (Input::GetInstance()->GetGamepadState(pad)) {
		if (Input::GetInstance()->TriggerButton(PadInput::B)) {
			if (!isChain_) {
				switch (comboIndex_) {
				case 0:
					Direction();
					isChain_ = true;
					break;
				case 1:
					Direction();
					isChain_ = true;
					break;
				case 2:
					Direction();
					isChain_ = true;
					break;
				case 3:
					Direction();
					isChain_ = true;
					break;
				default:
					break;
				}
			}
		}
	}

	switch (comboIndex_) {
	case 0:

		if (isAttack1_) {
			attackT1_ -= FPSKeeper::DeltaTime();

			if (attackT1_ <= 0.0f) {
				if (isChain_) {
					attackT1_ = 0.0f;
					attackT2_ = 40.0f;
					isAttack2_ = true;
					comboIndex_++;
					isChain_ = false;
					isAttack1_ = false;


					pPlayer_->GetWeaponModel()->transform.rotate.z = 1.53f;

				}
				else {
					isAttack_ = false;
					pPlayer_->SetIsAttack(false);
					pPlayer_->SetBehaviorRequest(Player::PlayerBehavior::kDefult);
					EndInit();
				}
			}
			else {
				const float kCharacterSpeed = 0.0f;
				Vector3 move = { 0.0f, 0.0f, 1.0f };
				move = move.Normalize() * kCharacterSpeed;
				Matrix4x4 rotatePlayer = MakeRotateXYZMatrix(pPlayer_->GetModel()->transform.rotate);
				move = TransformNormal(move, rotatePlayer);
				pPlayer_->GetModel()->transform.translate += move * FPSKeeper::DeltaTime();
			}

			if (attackT1_ >= 20.0f) {
				float t = 1.0f / 20.0f * (20.0f - (attackT1_ - 20.0f));
				Vector3 attackCollider = CatmullRom(attackPoint1_, t);
				pPlayer_->GetAABBAttack()->SetPos(attackCollider);
				pPlayer_->GetWeaponModel()->transform.translate = { attackCollider };

				pPlayer_->GetWeaponModel()->transform.rotate.x = LerpShortAngle(-0.23f, 2.0f, t);

			}
			else {
				pPlayer_->GetAABBAttack()->SetPos(attackPoint1_[3]);
			}

		}

		break;
	case 1:

		if (isAttack2_) {
			attackT2_ -= FPSKeeper::DeltaTime();

			if (attackT2_ <= 0.0f) {
				if (isChain_) {
					attackT2_ = 0.0f;
					attackT3_ = 40.0f;
					isAttack3_ = true;
					comboIndex_++;
					isAttack2_ = false;
					isChain_ = false;
					/*pPlayer_->GetAABBAttack()->SetWidth(3.0f);
					pPlayer_->GetAABBAttack()->SetHeight(1.0f);
					pPlayer_->GetAABBAttack()->SetDepth(1.0f);*/
				}
				else {
					isAttack_ = false;
					pPlayer_->SetIsAttack(false);
					pPlayer_->SetBehaviorRequest(Player::PlayerBehavior::kDefult);
					isAttack2_ = false;
					EndInit();
				}
			}
			else {
				const float kCharacterSpeed = 0.0f;
				Vector3 move = { 0.0f, 0.0f, 1.0f };
				move = move.Normalize() * kCharacterSpeed;
				Matrix4x4 rotatePlayer = MakeRotateXYZMatrix(pPlayer_->GetModel()->transform.rotate);
				move = TransformNormal(move, rotatePlayer);
				pPlayer_->GetModel()->transform.translate += move * FPSKeeper::DeltaTime();

			}

			if (attackT2_ >= 20.0f) {
				float t = 1.0f / 20.0f * (20.0f - (attackT2_ - 20.0f));
				Vector3 attackCollider = CatmullRom(attackPoint2_, t);
				pPlayer_->GetAABBAttack()->SetPos(attackCollider);
				pPlayer_->GetWeaponModel()->transform.translate = { attackCollider };

				pPlayer_->GetWeaponModel()->transform.rotate.x = LerpShortAngle(0.57f, 2.5f, t);

			}
			else {
				pPlayer_->GetAABBAttack()->SetPos(attackPoint2_[3]);
			}

		}

		break;
	case 2:

		if (isAttack3_) {
			attackT3_ -= FPSKeeper::DeltaTime();

			if (attackT3_ <= 0.0f) {
				if (isChain_) {
					attackT3_ = 0.0f;
					attackT4_ = 40.0f;
					isAttack4_ = true;
					comboIndex_++;
					isChain_ = false;
					isAttack3_ = false;
					/*pPlayer_->GetAABBAttack()->SetWidth(4.0f);
					pPlayer_->GetAABBAttack()->SetHeight(1.0f);
					pPlayer_->GetAABBAttack()->SetDepth(2.0f);*/
				}
				else {
					isAttack_ = false;
					pPlayer_->SetIsAttack(false);
					pPlayer_->SetBehaviorRequest(Player::PlayerBehavior::kDefult);
					EndInit();
				}
			}
			else {
				const float kCharacterSpeed = 0.0f;
				Vector3 move = { 0.0f, 0.0f, 1.0f };
				move = move.Normalize() * kCharacterSpeed;
				Matrix4x4 rotatePlayer = MakeRotateXYZMatrix(pPlayer_->GetModel()->transform.rotate);
				move = TransformNormal(move, rotatePlayer);
				pPlayer_->GetModel()->transform.translate += move * FPSKeeper::DeltaTime();
			}

			if (attackT3_ >= 20.0f) {
				float t = 1.0f / 20.0f * (20.0f - (attackT3_ - 20.0f));
				Vector3 attackCollider = CatmullRom(attackPoint3_, t);
				pPlayer_->GetAABBAttack()->SetPos(attackCollider);
				pPlayer_->GetWeaponModel()->transform.translate = { attackCollider };
			}
			else {
				pPlayer_->GetAABBAttack()->SetPos(attackPoint3_[3]);
			}

		}

		break;
	case 3:

		if (isAttack4_) {
			attackT4_ -= FPSKeeper::DeltaTime();

			if (attackT4_ <= 0.0f) {
				if (isChain_) {
					isAttack_ = false;
					pPlayer_->SetIsAttack(false);
					pPlayer_->SetBehaviorRequest(Player::PlayerBehavior::kDefult);
					EndInit();
					isChain_ = false;
				}
				else {
					isAttack_ = false;
					pPlayer_->SetIsAttack(false);
					pPlayer_->SetBehaviorRequest(Player::PlayerBehavior::kDefult);
					EndInit();
				}
			}
			else {
				const float kCharacterSpeed = 0.0f;
				Vector3 move = { 0.0f, 0.0f, 1.0f };
				move = move.Normalize() * kCharacterSpeed;
				Matrix4x4 rotatePlayer = MakeRotateXYZMatrix(pPlayer_->GetModel()->transform.rotate);
				move = TransformNormal(move, rotatePlayer);
				pPlayer_->GetModel()->transform.translate += move * FPSKeeper::DeltaTime();
			}

			if (attackT4_ >= 20.0f) {
				float t = 1.0f / 20.0f * (20.0f - (attackT4_ - 20.0f));
				Vector3 attackCollider = CatmullRom(attackPoint4_, t);
				pPlayer_->GetAABBAttack()->SetPos(attackCollider);
				pPlayer_->GetWeaponModel()->transform.translate = { attackCollider };
			}
			else {
				pPlayer_->GetAABBAttack()->SetPos(attackPoint4_[3]);
			}

		}

		break;
	default:
		break;
	}


	if (isAttack2_) {
		/*pPlayer_->GetAABBAttack()->SetPos({ 0.0f,0.0f,0.0f });*/
	}

}

void PlayerAttackBehavior::Direction() {
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
			pPlayer_->GetTrans().rotate.y = LerpShortAngle(pPlayer_->GetTrans().rotate.y, targetRotate, 1.0f);

		}

	}
}

void PlayerAttackBehavior::EndInit() {
	pPlayer_->GetWeaponModel()->transform.translate = { 1.0f,0.5f,0.0f };
	pPlayer_->GetWeaponModel()->transform.rotate = { 0.0f,0.0f,0.0f };
}



/// 長押し攻撃用
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