#pragma once
#include "Game/OriginObject/OriginGameObject.h"
#include "Game/Collider/AABBCollider.h"
#include "Engine/Particle/ParticleEmitter.h"


/// <summary>
/// BossItem:UnderRingクラス
/// </summary>
class UnderRing : public OriginGameObject {
public:
	UnderRing();
	~UnderRing() = default;

	void Initialize()override;
	void Update()override;
	void Draw(Graphics::Material* mate = nullptr, bool is = false)override;
	void DrawCollider();
	void DebugGUI()override;
	void ParameterGUI();

	/// <summary>値の初期化</summary>
	void InitParameter();

	/// <summary>リングの初期化</summary>
	void InitRing(const Math::Vector3& pos, float lifeT = 300.0f);

	//========================================================================*/
	//* Collision
	void OnCollisionEnter([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionStay([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionExit([[maybe_unused]] const ColliderInfo& other);

	//========================================================================*/
	//* Setter
	void SetIsLive(bool is) { isLive_ = is; }

	//========================================================================*/
	//* Getter
	BaseCollider* GetCollider() { return collider_.get(); }
	float GetLifeTime() { return lifeTime_; }
	bool GetIsLive() { return isLive_; }
	float GetRingRadMax() { return ringRadMax_; }
	float GetRingRadMin() { return ringRadMin_; }

private:

private:

	std::unique_ptr<Graphics::Object3d> cylinder_;

	bool isLive_ = false;
	float lifeTime_ = 300.0f;
	float speed_;
	float ringSize_;

	float ringRadMax_;
	float ringRadMin_;

	float uvTransX_;

	std::unique_ptr<AABBCollider> collider_;

};
