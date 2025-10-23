#pragma once
#include "Game/OriginObject/OriginGameObject.h"
#include "Game/Collider/AABBCollider.h"
#include "Engine/Particle/ParticleEmitter.h"


/// <summary>
/// BossItem:WaveWallクラス
/// </summary>
class WaveWall : public OriginGameObject {
public:
	WaveWall();
	~WaveWall() = default;

	void Initialize()override;
	void Update()override;
	void Draw(Material* mate = nullptr, bool is = false)override;
	void DrawCollider();
	void DebugGUI()override;
	void ParameterGUI();

	/// <summary>値の初期化</summary>
	void InitParameter();

	/// <summary>波攻撃の初期化</summary>
	void InitWave(const Vector3& pos, const Vector3& velo);
	/// <summary>波攻撃の進行方向更新</summary>
	void CalculetionFollowVec(const Vector3& target);

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


private:

private:

	std::unique_ptr<Object3d> underRing_;
	std::unique_ptr<Object3d> wave1_;
	std::unique_ptr<Object3d> wave2_;
	std::unique_ptr<Object3d> wave3_;

	bool isLive_ = false;
	float lifeTime_ = 300.0f;
	float speed_;
	Vector3 velocity_;

	float uvTransX_;

	std::unique_ptr<AABBCollider> collider_;

	// emitter
	ParticleEmitter spark1_;
	ParticleEmitter spark2_;


};
