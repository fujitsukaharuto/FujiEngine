#pragma once
#include "Game/OriginObject/OriginGameObject.h"
#include "Game/Collider/AABBCollider.h"

#include "Engine/Graphics/Particle/ParticleEmitter.h"

/// <summary>
/// Playerの弾クラス
/// </summary>
class PlayerBullet : public OriginGameObject {
public:
	PlayerBullet();
	~PlayerBullet();

	void Initialize()override;
	void Update()override;
	void Draw(bool is = false)override;

	/// <summary>
	/// パラメーターの初期化
	/// </summary>
	void InitParameter(const Math::Vector3& pos);
	/// <summary>
	/// 弾の進行方向の変更
	/// </summary>
	void CalculationFollowVec(const Math::Vector3& target);

	//========================================================================*/
	//* Collision
	void OnCollisionEnter([[maybe_unused]] const ColliderInfo& other)override;
	void OnCollisionStay([[maybe_unused]] const ColliderInfo& other)override;
	void OnCollisionExit([[maybe_unused]] const ColliderInfo& other)override;

	//========================================================================*/
	//* Charge
	/// <summary>
	///	チャージの処理
	/// </summary>
	void Charge(const Math::Vector3& pos, const Math::Vector3& rot);
	/// <summary>
	///	弾の強化
	/// </summary>
	void StrengthBullet();

	//========================================================================*/
	//* Release
	/// <summary>
	///	弾が発射される時の処理
	/// </summary>
	void Release(float speed,float damage,const Math::Vector3& velo);

	//========================================================================*/
	//* Getter
	bool GetIsLive() { return isLive_; }
	bool GetIsCharge() { return isCharge_; }
	bool GetIsStrength() { return isStrength_; }
	float GetDamage() { return damage_; }

	//========================================================================*/
	//* Setter
	void SetIsLive(bool is) { isLive_ = is; }

private:

	void ParticleEmitterSetting();

private:

	/// <summary>所有権は基底の colliders_ が持つ</summary>
	AABBCollider* collider_ = nullptr;

	Graphics::ParticleEmitter trajectory;
	Graphics::ParticleEmitter trajectory2;

	Graphics::ParticleEmitter hit_;
	Graphics::ParticleEmitter hit2_;
	Graphics::ParticleEmitter hit3_;
	Graphics::ParticleEmitter hitSmoke_;
	Graphics::ParticleEmitter hitCircle_;;


	bool isLive_ = false;
	bool isCharge_ = false;
	bool isStrength_ = false;

	Math::Vector3 velocity_;
	float speed_;
	float damage_;

	float zRotate_ = 0.0f;

};
