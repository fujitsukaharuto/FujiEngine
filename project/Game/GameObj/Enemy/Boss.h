#pragma once
#include "Game/OriginGameObject.h"
#include "Game/Collider/AABBCollider.h"

#include "Game/GameObj/Enemy/Behavior/BaseBossBehavior.h"

class Boss : public OriginGameObject {
public:
	Boss();
	~Boss();

	void Initialize()override;
	void Update()override;
	void Draw(Material* mate = nullptr)override;
	void DebugGUI()override;

	//========================================================================*/
	//* Behavior
	void ChangeBehavior(std::unique_ptr<BaseBossBehavior>behavior);

private:

private:

	std::unique_ptr<BaseBossBehavior> behavior_ = nullptr;

	std::unique_ptr<Object3d> shadow_;
	std::unique_ptr<AABBCollider> collider_;

};
