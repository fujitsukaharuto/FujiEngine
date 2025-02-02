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

			/*//現在のプレイヤーの座標
			Vector3 playerPos = pPlayer_->GetTrans().translate;
			// 移動後の座標を計算
			Vector3 newPos = playerPos + velo * FPSKeeper::DeltaTime();
			// 原点からの距離（円の半径方向の長さ）を計算
			float newPosLength = std::sqrt(newPos.x * newPos.x + newPos.z * newPos.z);
			// もし円の外に出るなら、velo を調整して境界上を滑るようにする
			if (newPosLength > mapMaxSize_) {
				// 法線ベクトル（円の中心方向）
				Vector3 normal = Vector3(newPos.x, 0.0f, newPos.z).Normalize();
				// velo の円の中心方向成分を除去（垂直成分のみ残す）
				float dot = velo.Dot(normal);  // velo の円の中心方向の成分
				velo -= normal * dot;          // 円の中心方向の動きを打ち消す
			}
			
			pPlayer_->GetTrans().translate = velo * FPSKeeper::DeltaTime();
			pPlayer_->SetVelocity(velo);*/


			// プレイヤーの新しい座標を計算
			Vector3 newPos = pPlayer_->GetTrans().translate + velo * FPSKeeper::DeltaTime();

			// 新しい座標の長さ（原点からの距離）を計算
			float newPosLength = newPos.Length();

			// 半径制限を超えていたら、38.5の円の内側に補正
			if (newPosLength > mapMaxSize_) {
				Vector3 correctionDir = newPos.Normalize(); // 中心方向の単位ベクトル
				newPos = correctionDir * mapMaxSize_; // 半径38.5の位置に補正
			}


			pPlayer_->GetTrans().translate = newPos;
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
