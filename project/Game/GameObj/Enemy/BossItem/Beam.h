#pragma once
#include "Game/OriginGameObject.h"
#include "Game/Collider/AABBCollider.h"
#include "Engine/Particle/ParticleEmitter.h"


class Boss;

class Beam : public OriginGameObject {
public:
	Beam();
	~Beam() = default;

	void Initialize()override;
	void Update()override;
	void Draw(Material* mate = nullptr, bool is = false)override;
	void DebugGUI()override;
	void ParameterGUI();

	void InitParameter();

	void InitBeam(const Vector3& pos, const Vector3& velo);
	bool BeamRotate();

	//========================================================================*/
	//* Collision
	void OnCollisionEnter([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionStay([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionExit([[maybe_unused]] const ColliderInfo& other);


	BaseCollider* GetCollider() { return collider_.get(); }
	float GetLifeTime() { return lifeTime_; }
	bool GetIsLive() { return isLive_; }

	void SetIsLive(bool is) { isLive_ = is; }
	void SetBossParent(Boss* boss);

private:

private:

	std::unique_ptr<Object3d> beamCore1_;
	std::unique_ptr<Object3d> beamCore2_;
	std::unique_ptr<Object3d> beamCore3_;
	std::unique_ptr<Object3d> beam1_;
	std::unique_ptr<Object3d> beam2_;
	std::unique_ptr<Object3d> beam3_;
	std::unique_ptr<Object3d> particleParent_;

	bool isLive_ = false;
	float lifeTime_ = 420.0f;

	float uvTransX_;

	float beam1BaseScale_;
	float beam2BaseScale_;
	float beam3BaseScale_;

	std::unique_ptr<AABBCollider> collider_;

	// emitter
	ParticleEmitter spark1_;
	ParticleEmitter spark2_;
	ParticleEmitter* beamParticle_;
	ParticleEmitter* beamLight_;

};
