#pragma once
#include "BasePlayerBehavior.h"


class PlayerRoot : public BasePlayerBehavior {
public:
	PlayerRoot(Player* pPlayer);
	~PlayerRoot();

	enum class Step {
		ROOT,
		TOJUMP,
	};

	void Update()override;
	void Debug()override;

private:

	Step step_;
	float speed_;

};
