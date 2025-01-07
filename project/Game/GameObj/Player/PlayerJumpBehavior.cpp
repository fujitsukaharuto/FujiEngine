#include "PlayerJumpBehavior.h"
#include "Game/GameObj/Player/Player.h"
#include "Input/Input.h"
#include "Math/MatrixCalculation.h"
#include "Camera/CameraManager.h"


PlayerJumpBehavior::PlayerJumpBehavior(Player* pPlayer) : pPlayer_(pPlayer) {

}

PlayerJumpBehavior::~PlayerJumpBehavior() {
}

void PlayerJumpBehavior::Initialize() {
}

void PlayerJumpBehavior::Update() {
	Jump();
}

void PlayerJumpBehavior::Jump() {

	if (isCharge_) {
		pPlayer_->GetBodyModel()->transform.scale.y -= 0.04f * FPSKeeper::DeltaTime();
		pPlayer_->GetBodyModel()->transform.scale.x += 0.04f * FPSKeeper::DeltaTime();
		pPlayer_->GetBodyModel()->transform.scale.z += 0.04f * FPSKeeper::DeltaTime();
		if (pPlayer_->GetBodyModel()->transform.scale.y <= 0.3f) {
			pPlayer_->GetBodyModel()->transform.scale.y = 0.3f;
			pPlayer_->GetBodyModel()->transform.scale.x = 0.7f;
			pPlayer_->GetBodyModel()->transform.scale.z = 0.7f;
			isCharge_ = false;
			isFly_ = true;
			Audio::GetInstance()->SoundPlayWave(pPlayer_->jumpSound);
		}
	}
	else if (isFly_) {
		if (pPlayer_->GetBodyModel()->transform.scale.y < 0.7f) {
			pPlayer_->GetBodyModel()->transform.scale.y += 0.08f * FPSKeeper::DeltaTime();
			pPlayer_->GetBodyModel()->transform.scale.x -= 0.08f * FPSKeeper::DeltaTime();
			pPlayer_->GetBodyModel()->transform.scale.z -= 0.08f * FPSKeeper::DeltaTime();
			if (pPlayer_->GetBodyModel()->transform.scale.y >= 0.7f) {
				pPlayer_->GetBodyModel()->transform.scale.y = 0.7f;
				pPlayer_->GetBodyModel()->transform.scale.x = 0.3f;
				pPlayer_->GetBodyModel()->transform.scale.z = 0.3f;
			}
		}

		/*accele_ -= kGra_ * FPSKeeper::DeltaTime();
		velo_ += accele_ * FPSKeeper::DeltaTime();*/
		pPlayer_->GetModel()->transform.translate.y += velo_ * FPSKeeper::DeltaTime();
		if (pPlayer_->GetModel()->transform.translate.y >= 10.0f) {
			pPlayer_->GetModel()->transform.translate.y = 10.0f;
			isFly_ = false;
			isSky_ = true;
		}
	}
	else if (isSky_) {
		isSky_ = false;
		pPlayer_->SetIsAttack(true);
		pPlayer_->GetAABBAttack()->SetTag("attack_knock");
		pPlayer_->GetAABBAttack()->SetPos({ 0.0f,0.0f,0.0f });
		pPlayer_->GetAABBAttack()->SetWidth(6.0f);
		pPlayer_->GetAABBAttack()->SetHeight(0.5f);
		pPlayer_->GetAABBAttack()->SetDepth(6.0f);
		pPlayer_->GetFireModel()->transform.scale.x = 5.0f;
		pPlayer_->GetWeaponModel()->transform.rotate.x = 1.56f;
		pPlayer_->GetWeaponModel()->transform.translate = { 0.5f,0.5f,1.0f };
		Audio::GetInstance()->SoundPlayWave(pPlayer_->attackSound3);
		isFall_ = true;
	}
	else if (isFall_) {
		pPlayer_->GetModel()->transform.translate.y -= (velo_ * 1.5f) * FPSKeeper::DeltaTime();
		if (pPlayer_->GetModel()->transform.translate.y <= 0.0f) {
			pPlayer_->GetModel()->transform.translate.y = 0.0f;
			isFall_ = false;
			isUnCharge_ = true;
			pPlayer_->EmitJumpAttack();
		}

		if (pPlayer_->GetFireModel()->transform.scale.x > 0.0f) {
			pPlayer_->GetFireModel()->transform.scale.x -= 0.12f;
			if (pPlayer_->GetFireModel()->transform.scale.x < 0.0f) {
				pPlayer_->GetFireModel()->transform.scale.x = 0.0f;
			}
		}
	}
	else if (isUnCharge_) {
		if (pPlayer_->GetBodyModel()->transform.scale.y > 0.2f) {
			pPlayer_->GetBodyModel()->transform.scale.y -= 0.08f * FPSKeeper::DeltaTime();
			pPlayer_->GetBodyModel()->transform.scale.x += 0.08f * FPSKeeper::DeltaTime();
			pPlayer_->GetBodyModel()->transform.scale.z += 0.08f * FPSKeeper::DeltaTime();
			if (pPlayer_->GetBodyModel()->transform.scale.y <= 0.2f) {
				pPlayer_->GetBodyModel()->transform.scale.y = 0.2f;
				pPlayer_->GetBodyModel()->transform.scale.x = 0.8f;
				pPlayer_->GetBodyModel()->transform.scale.z = 0.8f;
				isUnCharge_ = false;
				isReverse_ = true;
			}
		}
		
		if (pPlayer_->GetFireModel()->transform.scale.x > 0.0f) {
			pPlayer_->GetFireModel()->transform.scale.x -= 0.12f;
			if (pPlayer_->GetFireModel()->transform.scale.x < 0.0f) {
				pPlayer_->GetFireModel()->transform.scale.x = 0.0f;
			}
		}
	}
	else if (isReverse_) {
		pPlayer_->GetBodyModel()->transform.scale.y += 0.04f * FPSKeeper::DeltaTime();
		pPlayer_->GetBodyModel()->transform.scale.x -= 0.04f * FPSKeeper::DeltaTime();
		pPlayer_->GetBodyModel()->transform.scale.z -= 0.04f * FPSKeeper::DeltaTime();
		if (pPlayer_->GetBodyModel()->transform.scale.y >= 0.5f) {
			pPlayer_->GetBodyModel()->transform.scale.y = 0.5f;
			pPlayer_->GetBodyModel()->transform.scale.x = 0.5f;
			pPlayer_->GetBodyModel()->transform.scale.z = 0.5f;

			pPlayer_->SetIsAttack(false);

			isReverse_ = false;
			isStop_ = true;
		}
	}
	else if (isStop_) {
		stopTime_ -= FPSKeeper::DeltaTime();

		if (pPlayer_->GetFireModel()->transform.scale.x > 0.0f) {
			pPlayer_->GetFireModel()->transform.scale.x -= 0.12f;
			if (pPlayer_->GetFireModel()->transform.scale.x < 0.0f) {
				pPlayer_->GetFireModel()->transform.scale.x = 0.0f;
			}
		}

		if (stopTime_ <= 0.0f) {
			pPlayer_->SetBehaviorRequest(Player::PlayerBehavior::kDefult);
			EndInit();
		}
	}



}

void PlayerJumpBehavior::Direction() {
}

void PlayerJumpBehavior::EndInit() {
	pPlayer_->GetWeaponModel()->transform.translate = { 1.0f,0.5f,0.0f };
	pPlayer_->GetWeaponModel()->transform.rotate = { 0.0f,0.0f,0.0f };

	pPlayer_->SetIsAttack(false);
	pPlayer_->GetAABBAttack()->SetTag("attack");

	pPlayer_->GetBodyModel()->transform.translate.y = 0.0f;
	pPlayer_->GetBodyModel()->transform.rotate = { 0.0f,0.0f,0.0f };
	pPlayer_->GetBodyModel()->transform.scale = { 0.5f,0.5f,0.5f };
}


