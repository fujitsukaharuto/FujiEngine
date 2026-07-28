#pragma once
#include "Game/OriginObject/OriginGameObject.h"
#include "Game/Collider/AABBCollider.h"
#include "Engine/Graphics/Particle/ParticleEmitter.h"


/// <summary>
/// BossItem:WaveWallクラス
/// </summary>
class WaveWall : public OriginGameObject {
public:
	WaveWall();
	~WaveWall() = default;

	void Initialize()override;
	void Update()override;
	void Draw(bool is = false)override;
	void DrawCollider();
	void DebugGUI()override;
	void ParameterGUI();

	/// <summary>波攻撃の初期化</summary>
	void InitWave(const Math::Vector3& pos, const Math::Vector3& velo);
	/// <summary>波攻撃の進行方向更新</summary>
	void CalculationFollowVec(const Math::Vector3& target);

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


private:

	/// <summary>値の初期化</summary>
	void InitParameter();
	void InitEmitter();

private:

	Graphics::Object3d* underRing_ = nullptr;
	Graphics::Object3d* wave1_ = nullptr;
	Graphics::Object3d* wave2_ = nullptr;
	Graphics::Object3d* wave3_ = nullptr;

	bool isLive_ = false;
	float lifeTime_ = 300.0f;
	float speed_;
	Math::Vector3 velocity_;

	float uvTransX_;

	/// <summary>所有権は基底の colliders_ が持つ</summary>
	AABBCollider* collider_ = nullptr;

	// emitter
	Graphics::ParticleEmitter spark1_;
	Graphics::ParticleEmitter spark2_;


};
