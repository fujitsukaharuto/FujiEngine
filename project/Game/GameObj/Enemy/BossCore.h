#pragma once
#include "Engine/GameObject/GameObject.h"
#include "Engine/Collision/AABBCollider.h"

class Boss;

/// <summary>
/// Bossのあたり判定になるコア部分のクラス
/// </summary>
class BossCore : public GameObject::GameObject {
public:
	BossCore(Boss* pboss);
	~BossCore() = default;

	void Initialize()override;
	void Update()override;
	void Draw(bool is = false)override;
	void DebugGUI()override;
	void ParameterGUI();

	void InitParameter();


	//========================================================================*/
	//* Collision
	void OnCollisionEnter([[maybe_unused]] const Collision::ColliderInfo& other)override;
	void OnCollisionStay([[maybe_unused]] const Collision::ColliderInfo& other)override;
	void OnCollisionExit([[maybe_unused]] const Collision::ColliderInfo& other)override;

private:

private:

	Boss* pBoss_;

	/// <summary>所有権は基底の colliders_ が持つ</summary>
	Collision::AABBCollider* collider_ = nullptr;

};
