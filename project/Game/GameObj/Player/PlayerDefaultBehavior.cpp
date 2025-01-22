#include "PlayerDefaultBehavior.h"
#include "Game/GameObj/Player/Player.h"
#include "Input/Input.h"
#include "Math/MatrixCalculation.h"
#include "Camera/CameraManager.h"

PlayerDefaultBehavior::PlayerDefaultBehavior(Player* pPlayer) : pPlayer_(pPlayer) {
}

PlayerDefaultBehavior::~PlayerDefaultBehavior() {
}

void PlayerDefaultBehavior::Initialize() {
}

void PlayerDefaultBehavior::Update() {
	Move();
}

void PlayerDefaultBehavior::Move() {

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
			velo = velo.Normalize() * kCharacterSpeed_;

			Vector3 cameraR = CameraManager::GetInstance()->GetCamera()->transform.rotate;
			cameraR.x = 0.0f;
			Matrix4x4 rotateCamera = MakeRotateXYZMatrix(cameraR);
			velo = TransformNormal(velo, rotateCamera);

			pPlayer_->GetTrans().translate += velo * FPSKeeper::DeltaTime();
			pPlayer_->SetVelocity(velo);

			float targetRotate = std::atan2(velo.x, velo.z);
			pPlayer_->GetTrans().rotate.y = LerpShortAngle(pPlayer_->GetTrans().rotate.y, targetRotate, 0.075f);

			if (animeTime_ == 0.0f) {
				pPlayer_->EmitWalk();
			}
			animeTime_ += FPSKeeper::DeltaTime();
			if (animeTime_ > animeTimeLimitte_) {
				animeTime_ = 0.0f;
			}
			float t = 1.0f / animeTimeLimitte_ * animeTime_;
			float tt = (4.0f * t * (1.0f - t));
			pPlayer_->GetBodyModel()->transform.translate.y = Lerp(0.0f, 0.25f, tt);
			pPlayer_->GetBodyModel()->transform.scale.y = Lerp(0.4f, 0.6f, tt);
			pPlayer_->GetBodyModel()->transform.scale.x = Lerp(0.6f, 0.4f, tt);
			pPlayer_->GetBodyModel()->transform.scale.z = Lerp(0.6f, 0.4f, tt);
		}
		else {
			if (animeTime_ != 0.0f) {
				if (animeTime_ >= (animeTimeLimitte_ * 0.5f)) {
					animeTime_ += FPSKeeper::DeltaTime();
					if (animeTime_ > animeTimeLimitte_) {
						animeTime_ = 0.0f;
					}
				}
				else {
					animeTime_ -= FPSKeeper::DeltaTime();
					if (animeTime_ < 0.0f) {
						animeTime_ = 0.0f;
					}
				}
				float t = 1.0f / animeTimeLimitte_ * animeTime_;
				float tt = (4.0f * t * (1.0f - t));
				pPlayer_->GetBodyModel()->transform.translate.y = Lerp(0.0f, 0.4f, tt);
				pPlayer_->GetBodyModel()->transform.scale.y = Lerp(0.4f, 0.6f, tt);
				pPlayer_->GetBodyModel()->transform.scale.x = Lerp(0.6f, 0.4f, tt);
				pPlayer_->GetBodyModel()->transform.scale.z = Lerp(0.6f, 0.4f, tt);
			}
			else {
				pPlayer_->GetBodyModel()->transform.translate.y = Lerp(pPlayer_->GetBodyModel()->transform.translate.y, 0.0f, 0.1f);
				pPlayer_->GetBodyModel()->transform.scale.y = Lerp(pPlayer_->GetBodyModel()->transform.scale.y, 0.5f, 0.1f);
				pPlayer_->GetBodyModel()->transform.scale.x = Lerp(pPlayer_->GetBodyModel()->transform.scale.x, 0.5f, 0.1f);
				pPlayer_->GetBodyModel()->transform.scale.z = Lerp(pPlayer_->GetBodyModel()->transform.scale.z, 0.5f, 0.1f);
			}


		}

	}

}
