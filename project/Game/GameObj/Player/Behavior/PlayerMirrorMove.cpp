#include "PlayerMirrorMove.h"

#include "Engine/Input/Input.h"

#include "Game/Objects3D/Player/Player.h"
#include "Game/Objects3D/Player/Behavior/PlayerRoot.h"

PlayerMirrorMove::PlayerMirrorMove(Player* pPlayer) : BasePlayerBehavior(pPlayer) {
	step_ = Step::MIRRORMOVE;
}

PlayerMirrorMove::~PlayerMirrorMove() {
}

void PlayerMirrorMove::Update() {

	switch (step_) {
	case PlayerMirrorMove::Step::MIRRORMOVE:

		step_ = Step::TOROOT;

		break;
	case PlayerMirrorMove::Step::TOROOT:
		pPlayer_->ChangeBehavior(std::make_unique<PlayerRoot>(pPlayer_));
		break;
	default:
		break;
	}

}

void PlayerMirrorMove::Debug() {
}
