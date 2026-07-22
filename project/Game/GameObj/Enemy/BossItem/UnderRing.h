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
	void Draw(bool is = false)override;
	void DrawCollider();
	void DebugGUI()override;
	void ParameterGUI();

	/// <summary>値の初期化</summary>
	void InitParameter();

	/// <summary>リングの初期化</summary>
	void InitRing(const Math::Vector3& pos, float lifeT = 300.0f);

	//========================================================================*/
	//* Collision
	void OnCollisionEnter([[maybe_unused]] const ColliderInfo& other)override;
	void OnCollisionStay([[maybe_unused]] const ColliderInfo& other)override;
	void OnCollisionExit([[maybe_unused]] const ColliderInfo& other)override;

	//========================================================================*/
	//* Setter
	void SetIsLive(bool is) { isLive_ = is; }

	//========================================================================*/
	//* Getter
	float GetLifeTime() { return lifeTime_; }
	bool GetIsLive() { return isLive_; }
	float GetRingRadMax() { return ringRadMax_; }
	float GetRingRadMin() { return ringRadMin_; }

private:

private:

	/// <summary>追加ビジュアル。所有権は基底の renderers_ が持つ</summary>
	Graphics::Object3d* cylinder_ = nullptr;

	bool isLive_ = false;
	float lifeTime_ = 300.0f;
	float speed_;
	float ringSize_;

	float ringRadMax_;
	float ringRadMin_;
	float maxScale_ = 0.5f;
	float minScale_ = 0.435f;

	float uvTransX_;

	/// <summary>所有権は基底の colliders_ が持つ</summary>
	AABBCollider* collider_ = nullptr;

};
