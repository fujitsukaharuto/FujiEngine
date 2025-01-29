#include "PlayerAttackBehavior.h"
#include "Game/GameObj/Player/Player.h"
#include "Input/Input.h"
#include "Math/MatrixCalculation.h"
#include "Model/PlaneDrawer.h"
#include "Camera/CameraManager.h"
#include "Game/GameObj/LockOn.h"


// やらなければならないこと
//
// ヒットストップ
// もっとエフェクト
// 敵の攻撃、自機の喰らうモーション
// 移動制限
// シェイク
// 攻撃の追加
// UI
// 影
// 
// 
// 
// トレイルエフェクトに炎を
// スラッシュを複数色にする
// 
// 
// 





PlayerAttackBehavior::PlayerAttackBehavior(Player* pPlayer) : pPlayer_(pPlayer) {
	pPlayer_->GetAABBAttack()->SetWidth(1.4f);
	pPlayer_->GetAABBAttack()->SetHeight(1.4f);
	pPlayer_->GetAABBAttack()->SetDepth(1.4f);

	isAttack_ = true;
	pPlayer_->SetIsAttack(true);

	attackPoint1_ = {
		{0.0f,1.5f,0.75f},
		{0.0f,0.9f,1.0f},
		{0.0f,0.5f,1.25f},
		{0.0f,0.2f,1.2f},
	};
	attackPoint2_ = {
		{-1.0f,0.5f,0.5f},
		{-0.5f,0.5f,0.75f},
		{0.5f,0.5f,0.75f},
		{1.0f,0.5f,0.5f},
	};
	attackPoint3_ = {
		{1.0f,1.5f,0.2f},
		{0.5f,0.9f,0.7f},
		{-0.5f,0.5f,1.0f},
		{-1.0f,0.2f,1.0f},
	};
	attackPoint4_ = {
		{-1.0f,1.5f,0.2f},
		{-0.5f,0.9f,0.7f},
		{0.5f,0.5f,1.0f},
		{1.0f,0.2f,1.0f},
	}; 
	attackPoint5_ = {
		{1.0f,0.5f,0.0f},
		{0.5f,0.5f,0.5f},
		{-0.5f,0.5f,0.5f},
		{-1.0f,0.5f,0.0f},
	};

	pPlayer_->GetSlashModel()->transform.translate.y = 1.4f;
	pPlayer_->GetSlashModel()->transform.translate.z = 1.0f;
	pPlayer_->GetSlashModel()->transform.rotate.z = 1.53f;
	pPlayer_->GetSlashModel()->transform.rotate.y = 1.4f;

	Direction();
	Audio::GetInstance()->SoundPlayWave(pPlayer_->attackSound1);
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
		if (Input::GetInstance()->TriggerButton(PadInput::X)) {
			if (!isChain_) {
				isChain_ = true;
			}
		}
	}

	switch (comboIndex_) {
	case 0:

		if (isAttack1_) {
			attackT1_ -= FPSKeeper::DeltaTime();

			if (attackT1_ >= (40.0f - attackLimmit1_)) {

				Vector3 move = { 0.0f, 0.0f, 1.0f };
				move = move.Normalize() * attackSpeed_;
				Matrix4x4 rotatePlayer = MakeRotateXYZMatrix(pPlayer_->GetModel()->transform.rotate);
				move = TransformNormal(move, rotatePlayer);
				pPlayer_->GetModel()->transform.translate += move * FPSKeeper::DeltaTime();

				float t = 1.0f / attackLimmit1_ * (attackLimmit1_ - (attackT1_ - (40.0f-attackLimmit1_)));
				Vector3 attackCollider = CatmullRom(attackPoint1_, t);
				pPlayer_->GetAABBAttack()->SetPos({ attackCollider.x * 1.4f,attackCollider.y,attackCollider.z * 1.8f });
				pPlayer_->GetWeaponModel()->transform.translate = { attackCollider };

				pPlayer_->GetWeaponModel()->transform.rotate.x = LerpShortAngle(-0.23f, 2.0f, t);
				pPlayer_->GetSlashModel()->transform.rotate.y = Lerp(2.22f, 0.2f, t);

				pPlayer_->GetBodyModel()->transform.scale.y = Lerp(0.75f, 0.3f, t);
				pPlayer_->GetBodyModel()->transform.rotate.x = LerpShortAngle(-0.15f, 0.2f, t);
				float xzScale = Lerp(0.25f, 0.7f, t);
				pPlayer_->GetBodyModel()->transform.scale.x = xzScale;
				pPlayer_->GetBodyModel()->transform.scale.z = xzScale;

				pPlayer_->GetFireModel()->transform.scale.x += 0.16f;

			}
			else {
				pPlayer_->GetAABBAttack()->SetPos(attackPoint1_[3]);
				pPlayer_->SetIsAttack(false);
				if (pPlayer_->GetFireModel()->transform.scale.x > 0.0f) {
					pPlayer_->GetFireModel()->transform.scale.x -= 0.03f;
					if (pPlayer_->GetFireModel()->transform.scale.x < 0.0f) {
						pPlayer_->GetFireModel()->transform.scale.x = 0.0f;
					}
				}
			}

			if (attackT1_ <= 0.0f) {
				if (isChain_) {
					attackT1_ = 0.0f;
					attackT2_ = 40.0f;
					isAttack2_ = true;
					comboIndex_++;
					isChain_ = false;
					isAttack1_ = false;


					pPlayer_->GetWeaponModel()->transform.rotate.z = 1.53f;
					pPlayer_->GetBodyModel()->transform.scale = { 0.5f,0.5f,0.5f };
					pPlayer_->GetBodyModel()->transform.rotate.x = 0.0f;

					pPlayer_->GetSlashModel()->transform.translate.y = 0.5f;
					pPlayer_->GetSlashModel()->transform.translate.z = 1.0f;
					pPlayer_->GetSlashModel()->transform.rotate.z = 3.15f;
					pPlayer_->GetSlashModel()->transform.rotate.y = 1.6f;
					pPlayer_->SetSlashReSet(10.0f);

					PlaneDrawer::GetInstance()->Reset();
					Direction();
					Audio::GetInstance()->SoundPlayWave(pPlayer_->attackSound1);

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


		}

		break;
	case 1:

		if (isAttack2_) {
			attackT2_ -= FPSKeeper::DeltaTime();

			if (attackT2_ >= (40.0f - attackLimmit2_)) {
				float t = 1.0f / attackLimmit2_ * (attackLimmit2_ - (attackT2_ - (40.0f - attackLimmit2_)));
				Vector3 attackCollider = CatmullRom(attackPoint2_, t);
				pPlayer_->GetAABBAttack()->SetPos({ attackCollider.x * 1.8f,attackCollider.y,attackCollider.z * 2.4f });
				pPlayer_->GetWeaponModel()->transform.translate = { attackCollider };

				pPlayer_->GetWeaponModel()->transform.rotate.x = LerpShortAngle(0.57f, 2.5f, t);
				pPlayer_->GetSlashModel()->transform.rotate.y = Lerp(1.6f, -0.65f, t);

				pPlayer_->GetBodyModel()->transform.rotate.y = LerpShortAngle(-0.45f, 0.45f, t);
				pPlayer_->GetBodyModel()->transform.rotate.z= LerpShortAngle(0.15f, -0.15f, t);
				pPlayer_->GetBodyModel()->transform.scale.y = Lerp(0.45f, 0.35f, t);
				float xzScale = Lerp(0.55f, 0.65f, t);
				pPlayer_->GetBodyModel()->transform.scale.x = xzScale;
				pPlayer_->GetBodyModel()->transform.scale.z = xzScale;

				pPlayer_->GetFireModel()->transform.scale.x += 0.16f;
				pPlayer_->SetIsAttack(true);
			}
			else {
				pPlayer_->GetAABBAttack()->SetPos(attackPoint2_[3]);
				pPlayer_->SetIsAttack(false);

				if (pPlayer_->GetFireModel()->transform.scale.x > 0.0f) {
					pPlayer_->GetFireModel()->transform.scale.x -= 0.03f;
					if (pPlayer_->GetFireModel()->transform.scale.x < 0.0f) {
						pPlayer_->GetFireModel()->transform.scale.x = 0.0f;
					}
				}
			}

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

					pPlayer_->GetWeaponModel()->transform.rotate.z = -0.5f;
					pPlayer_->GetWeaponModel()->transform.rotate.y = -0.55f;
					pPlayer_->GetBodyModel()->transform.rotate.z = 0.0f;
					pPlayer_->GetBodyModel()->transform.scale = { 0.5f,0.5f,0.5f };

					pPlayer_->GetSlashModel()->transform.translate.y = 1.4f;
					pPlayer_->GetSlashModel()->transform.translate.z = 1.0f;
					pPlayer_->GetSlashModel()->transform.rotate.z = 0.8f;
					pPlayer_->GetSlashModel()->transform.rotate.y = 2.2f;
					pPlayer_->SetSlashReSet(10.0f);

					PlaneDrawer::GetInstance()->Reset();
					Direction();
					Audio::GetInstance()->SoundPlayWave(pPlayer_->attackSound1);

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

		}

		break;
	case 2:

		if (isAttack3_) {
			attackT3_ -= FPSKeeper::DeltaTime();

			if (attackT3_ >= (40.0f - attackLimmit3_)) {
				float t = 1.0f / attackLimmit3_ * (attackLimmit3_ - (attackT3_ - (40.0f - attackLimmit3_)));
				Vector3 attackCollider = CatmullRom(attackPoint3_, t);
				pPlayer_->GetAABBAttack()->SetPos({ attackCollider.x * 1.4f,attackCollider.y,attackCollider.z * 2.2f });
				pPlayer_->GetWeaponModel()->transform.translate = { attackCollider };

				pPlayer_->GetWeaponModel()->transform.rotate.x = LerpShortAngle(-0.23f, 2.0f, t);
				pPlayer_->GetSlashModel()->transform.rotate.y = Lerp(2.22f, 0.2f, t);


				pPlayer_->GetBodyModel()->transform.rotate.y = LerpShortAngle(0.35f, -0.35f, t);
				pPlayer_->GetBodyModel()->transform.rotate.x = LerpShortAngle(-0.15f, 0.2f, t);
				pPlayer_->GetBodyModel()->transform.scale.y = Lerp(0.65f, 0.4f, t);
				float xzScale = Lerp(0.35f, 0.6f, t);
				pPlayer_->GetBodyModel()->transform.scale.x = xzScale;
				pPlayer_->GetBodyModel()->transform.scale.z = xzScale;

				pPlayer_->GetFireModel()->transform.scale.x += 0.16f;
				pPlayer_->SetIsAttack(true);
			}
			else {
				pPlayer_->GetAABBAttack()->SetPos(attackPoint3_[3]);
				pPlayer_->SetIsAttack(false);

				if (pPlayer_->GetFireModel()->transform.scale.x > 0.0f) {
					pPlayer_->GetFireModel()->transform.scale.x -= 0.03f;
					if (pPlayer_->GetFireModel()->transform.scale.x < 0.0f) {
						pPlayer_->GetFireModel()->transform.scale.x = 0.0f;
					}
				}
			}

		}

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

				pPlayer_->GetWeaponModel()->transform.rotate.z = 0.5f;
				pPlayer_->GetWeaponModel()->transform.rotate.y = 0.55f;
				pPlayer_->GetBodyModel()->transform.scale = { 0.5f,0.5f,0.5f };

				pPlayer_->GetSlashModel()->transform.translate.y = 1.4f;
				pPlayer_->GetSlashModel()->transform.translate.z = 1.0f;
				pPlayer_->GetSlashModel()->transform.rotate.z = 2.4f;
				pPlayer_->GetSlashModel()->transform.rotate.y = 2.2f;
				pPlayer_->SetSlashReSet(10.0f);

				PlaneDrawer::GetInstance()->Reset();
				Direction();
				Audio::GetInstance()->SoundPlayWave(pPlayer_->attackSound1);

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

		break;
	case 3:

		if (isAttack4_) {
			attackT4_ -= FPSKeeper::DeltaTime();

			if (attackT4_ >= (40.0f - attackLimmit4_)) {
				float t = 1.0f / attackLimmit4_ * (attackLimmit4_ - (attackT4_ - (40.0f - attackLimmit4_)));
				Vector3 attackCollider = CatmullRom(attackPoint4_, t);
				pPlayer_->GetAABBAttack()->SetPos({ attackCollider.x * 1.4f,attackCollider.y,attackCollider.z * 2.2f });
				pPlayer_->GetWeaponModel()->transform.translate = { attackCollider };

				pPlayer_->GetWeaponModel()->transform.rotate.x = LerpShortAngle(-0.23f, 2.0f, t);
				pPlayer_->GetSlashModel()->transform.rotate.y = Lerp(2.22f, 0.2f, t);

				pPlayer_->GetBodyModel()->transform.rotate.y = LerpShortAngle(-0.35f, 0.35f, t);
				pPlayer_->GetBodyModel()->transform.rotate.x = LerpShortAngle(-0.15f, 0.2f, t);
				pPlayer_->GetBodyModel()->transform.scale.y = Lerp(0.65f, 0.4f, t);
				float xzScale = Lerp(0.35f, 0.6f, t);
				pPlayer_->GetBodyModel()->transform.scale.x = xzScale;
				pPlayer_->GetBodyModel()->transform.scale.z = xzScale;

				pPlayer_->GetFireModel()->transform.scale.x += 0.16f;
				pPlayer_->SetIsAttack(true);
			}
			else {
				pPlayer_->GetAABBAttack()->SetPos(attackPoint4_[3]);
				pPlayer_->SetIsAttack(false);

				if (pPlayer_->GetFireModel()->transform.scale.x > 0.0f) {
					pPlayer_->GetFireModel()->transform.scale.x -= 0.03f;
					if (pPlayer_->GetFireModel()->transform.scale.x < 0.0f) {
						pPlayer_->GetFireModel()->transform.scale.x = 0.0f;
					}
				}
			}

		}

		if (attackT4_ <= 0.0f) {
			if (isChain_) {
				attackT4_ = 0.0f;
				attackT5_ = 40.0f;
				isAttack5_ = true;
				comboIndex_++;
				isChain_ = false;
				isAttack4_ = false;
				pPlayer_->GetAABBAttack()->SetWidth(5.0f);
				pPlayer_->GetAABBAttack()->SetHeight(1.0f);
				pPlayer_->GetAABBAttack()->SetDepth(5.0f);

				pPlayer_->GetWeaponModel()->transform.rotate.z = -1.6f;
				pPlayer_->GetWeaponModel()->transform.rotate.y = -0.1f;
				pPlayer_->GetBodyModel()->transform.scale = { 0.5f,0.5f,0.5f };

				pPlayer_->GetAABBAttack()->SetTag("attack_knock");

				pPlayer_->GetSlashModel()->transform.translate.y = 0.5f;
				pPlayer_->GetSlashModel()->transform.translate.z = 1.0f;
				pPlayer_->GetSlashModel()->transform.rotate.z = 0.0f;
				pPlayer_->GetSlashModel()->transform.rotate.y = 2.2f;
				pPlayer_->SetSlashReSet(12.0f);

				PlaneDrawer::GetInstance()->Reset();
				Direction();
				Audio::GetInstance()->SoundPlayWave(pPlayer_->attackSound2);

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

		break;
	case 4:

		if (isAttack5_) {
			attackT5_ -= FPSKeeper::DeltaTime();

			if (attackT5_ >= (40.0f - attackLimmit5_)) {
				float t = 1.0f / attackLimmit5_ * (attackLimmit5_ - (attackT5_ - (40.0f - attackLimmit5_)));
				Vector3 attackCollider = CatmullRom(attackPoint5_, t);
				pPlayer_->GetAABBAttack()->SetPos({ attackCollider.x * 1.4f,attackCollider.y,attackCollider.z * 1.6f });
				pPlayer_->GetWeaponModel()->transform.translate = { attackCollider };

				pPlayer_->GetWeaponModel()->transform.rotate.x = LerpShortAngle(2.5f, 2.46f, t);
				pPlayer_->GetSlashModel()->transform.rotate.y = Lerp(1.0f, -0.2f, t);


				pPlayer_->GetModel()->transform.rotate.y -= 0.405f;
				pPlayer_->GetBodyModel()->transform.rotate.y = LerpShortAngle(-0.45f, 0.45f, t);
				pPlayer_->GetBodyModel()->transform.rotate.z = LerpShortAngle(-0.15f, 0.15f, t);
				pPlayer_->GetBodyModel()->transform.scale.y = Lerp(0.35f, 0.45f, t);
				float xzScale = Lerp(0.65f, 0.55f, t);
				pPlayer_->GetBodyModel()->transform.scale.x = xzScale;
				pPlayer_->GetBodyModel()->transform.scale.z = xzScale;

				pPlayer_->GetFireModel()->transform.scale.x += 0.18f;
				pPlayer_->SetIsAttack(true);
			}
			else {
				pPlayer_->GetAABBAttack()->SetPos(attackPoint5_[3]);
				pPlayer_->SetIsAttack(false);

				if (pPlayer_->GetFireModel()->transform.scale.x > 0.0f) {
					pPlayer_->GetFireModel()->transform.scale.x -= 0.03f;
					if (pPlayer_->GetFireModel()->transform.scale.x < 0.0f) {
						pPlayer_->GetFireModel()->transform.scale.x = 0.0f;
					}
				}
			}

		}

		if (attackT5_ <= 0.0f) {
			if (isChain_) {
				isAttack_ = false;
				pPlayer_->SetIsAttack(false);
				pPlayer_->SetBehaviorRequest(Player::PlayerBehavior::kDefult);
				EndInit();
				pPlayer_->GetFireModel()->transform.scale.x = 0.0f;
				isChain_ = false;
			}
			else {
				isAttack_ = false;
				pPlayer_->SetIsAttack(false);
				pPlayer_->SetBehaviorRequest(Player::PlayerBehavior::kDefult);
				EndInit();
				pPlayer_->GetFireModel()->transform.scale.x = 0.0f;
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

		break;
	default:
		break;
	}


	if (isAttack5_) {
		pPlayer_->GetAABBAttack()->SetPos({ 0.0f,0.0f,0.0f });
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


		if (pPlayer_->GetLockOn()) {
			Vector3 lockOnPosition = pPlayer_->GetLockOnPtr()->GetTargetPosition();
			Vector3 sub = lockOnPosition - pPlayer_->GetModel()->transform.translate;

			float distance = sub.Length();
			const float threshold2 = 2.0f;

			if (distance > threshold2) {
				pPlayer_->GetModel()->transform.rotate.y = std::atan2(sub.x, sub.z);

				if (attackSpeed_ > distance - threshold2) {
					attackSpeed_ = distance - threshold2;
				}
			}
		}

	}
}

void PlayerAttackBehavior::EndInit() {
	pPlayer_->GetWeaponModel()->transform.translate = { 1.0f,0.5f,0.0f };
	pPlayer_->GetWeaponModel()->transform.rotate = { 0.0f,0.0f,0.0f };

	pPlayer_->GetAABBAttack()->SetTag("attack");

	pPlayer_->GetBodyModel()->transform.rotate = { 0.0f,0.0f,0.0f };
	pPlayer_->GetBodyModel()->transform.scale = { 0.5f,0.5f,0.5f };
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