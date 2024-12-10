#pragma once
#include "Game/BaseGameObject.h"
#include "ImGuiManager/ImGuiManager.h"
#include "Game/Collider/BaseCollider.h"
#include "Game/Collider/AABBCollider.h"
#include "Math/MatrixCalculation.h"

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



	void SetVelocity(const Vector3& v) { velocity_ = v; }


#ifdef _DEBUG
	void Debug()override;
#endif // _DEBUG


private:

	std::unique_ptr<AABBCollider> collider_ = nullptr;

	Vector3 velocity_{};



};
