#pragma once

#include"BasePlayerBehavior.h"
#include"math/Easing.h"

class PlayerRecoil : public BasePlayerBehavior {
private:
	enum class Step {
		RECOIL,
		RETUNROOT,
	};
private:

	float speed_;
	/// ===================================================
	///private varians
	/// ===================================================
	Step step_;
	float jumpSpeed_;
	float playerDirectionX_;
	float recoilTime_;
public:
	//コンストラクタ
	PlayerRecoil(Player* boss);
	~PlayerRecoil();

	void Update()override;

	void Debug()override;

};
