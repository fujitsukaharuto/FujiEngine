#pragma once
#include "Game/BaseGameObject.h"
#include "ImGuiManager/ImGuiManager.h"
#include "Game/Collider/BaseCollider.h"
#include "Game/Collider/AABBCollider.h"


class Player : public BaseGameObject {
public:
	Player();
	~Player()override;

	enum class PlayerBehavior {
		kDefult,
		kAttack,
	};

	void Initialize()override;
	void Update()override;
	void Draw(Material* mate = nullptr)override;

	void OnCollisionEnter(const ColliderInfo& other);

	BaseCollider* GetCollider() { return collider_.get(); }

#ifdef _DEBUG
	void Debug()override;
#endif // _DEBUG


private:

	std::unique_ptr<AABBCollider> collider_ = nullptr;

};
