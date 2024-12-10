#include "PlayerDefaultBehavior.h"
#include "Game/GameObj/Player/Player.h"
#include "Input/Input.h"
#include "Math/MatrixCalculation.h"

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
		Vector3 velo = { padStickL.x / SHRT_MAX, 0, padStickL.y / SHRT_MAX };

		if (velo.Lenght() > threshold) {
			ismoving = true;
		}


		if (ismoving) {
			const float kCharacterSpeed = 0.15f;
			velo = velo.Normalize() * kCharacterSpeed;
			/*Matrix4x4 rotateCamera = MakeRotateXYZMatrix(viewProjection_->rotation_);
			velocity_ = TransformNormal(velocity_, rotateCamera);*/

			pPlayer_->GetTrans().translate += velo * FPSKeeper::DeltaTime();
			pPlayer_->SetVelocity(velo);

			float targetRotate = std::atan2(velo.x, velo.z);
			pPlayer_->GetTrans().rotate.y = LerpShortAngle(pPlayer_->GetTrans().rotate.y, targetRotate, 0.075f);

		}

	}

}
