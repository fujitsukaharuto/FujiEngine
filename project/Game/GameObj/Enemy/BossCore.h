#pragma once
#include "Game/OriginObject/OriginGameObject.h"
#include "Game/Collider/AABBCollider.h"

class Boss;

/// <summary>
/// Bossのあたり判定になるコア部分のクラス
/// </summary>
class BossCore : public OriginGameObject {
public:
	BossCore(Boss* pboss);
	~BossCore() = default;

	void Initialize()override;
	void Update()override;
	void Draw(Graphics::Material* mate = nullptr, bool is = false)override;
	void DebugGUI()override;
	void ParameterGUI();

	void InitParameter();

	//========================================================================*/
	//* Behavior
	//void ChangeBehavior(std::unique_ptr<BaseBossBehavior>behavior);

	//========================================================================*/
	//* Collision
	void OnCollisionEnter([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionStay([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionExit([[maybe_unused]] const ColliderInfo& other);

	/// <summary>
	///	コライダーの取得
	/// </summary>
	BaseCollider* GetCollider() { return collider_.get(); }

private:

private:

	Boss* pBoss_;

	//std::unique_ptr<BaseBossBehavior> behavior_ = nullptr;

	std::unique_ptr<AABBCollider> collider_;

};
