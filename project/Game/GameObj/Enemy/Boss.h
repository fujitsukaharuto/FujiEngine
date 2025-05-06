#pragma once
#include "Game/OriginGameObject.h"
#include "Game/Collider/AABBCollider.h"


class Boss : public OriginGameObject {
public:
	Boss();
	~Boss();

	void Initialize()override;
	void Update()override;
	void Draw(Material* mate = nullptr)override;
	void DebugGUI()override;



private:

};
