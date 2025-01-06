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

		}

	}

}
