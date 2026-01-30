#pragma once
#include "Game/OriginObject/OriginGameObject.h"
#include "Game/Collider/AABBCollider.h"

#include "Engine/Particle/ParticleEmitter.h"

/// <summary>
/// Playerの弾クラス
/// </summary>
class PlayerBullet : public OriginGameObject {
public:
	PlayerBullet();
	~PlayerBullet();

	void Initialize()override;
	void Update()override;
	void Draw(Graphics::Material* mate = nullptr, bool is = false)override;

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
	void OnCollisionEnter([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionStay([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionExit([[maybe_unused]] const ColliderInfo& other);

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
	BaseCollider* GetCollider() { return collider_.get(); }

	//========================================================================*/
	//* Setter
	void SetIsLive(bool is) { isLive_ = is; }

private:

	void ParticleEmitterSetting();

private:

	std::unique_ptr<AABBCollider> collider_;

	ParticleEmitter trajectory;
	ParticleEmitter trajectory2;

	ParticleEmitter hit_;
	ParticleEmitter hit2_;
	ParticleEmitter hit3_;
	ParticleEmitter hitSmoke_;
	ParticleEmitter hitCircle_;;


	bool isLive_ = false;
	bool isCharge_ = false;
	bool isStrength_ = false;

	Math::Vector3 velocity_;
	float speed_;
	float damage_;

	float zRotate_ = 0.0f;

};
