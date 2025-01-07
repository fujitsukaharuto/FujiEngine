#pragma once
#include "Game/OriginState.h"
#include "Audio/Audio.h"

class Player;


class PlayerJumpBehavior : public OriginState {
public:
	PlayerJumpBehavior(Player* pPlayer);
	~PlayerJumpBehavior()override;

	void Initialize()override;
	void Update()override;


	void Jump();
	void Direction();

	void EndInit();


private:

	Player* pPlayer_;


	// 変数
	float kCharacterSpeed_ = 0.15f;
	float kGra_ = 0.01f;

	// 飛行前
	bool isCharge_ = true;

	// 飛行中
	bool isFly_ = false;

	// 滞空
	bool isSky_ = false;

	// 降下中
	bool isFall_ = false;

	// 降下後
	bool isUnCharge_ = false;
	bool isReverse_ = false;

	// 硬直
	bool isStop_ = false;
	float stopTime_ = 20.0f;

	float velo_ = 0.5f;
	float accele_ = 0.0f;

};
