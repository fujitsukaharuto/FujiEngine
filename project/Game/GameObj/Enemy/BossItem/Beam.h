#pragma once
#include "Game/OriginObject/OriginGameObject.h"
#include "Game/Collider/AABBCollider.h"
#include "Engine/Particle/ParticleEmitter.h"


enum class BeamStep{
	AroundAttack,
	RotateBeam,
};

/// <summary>
/// ビームを構成するオブジェクト
/// </summary>
struct OneBeam {
	std::unique_ptr<Graphics::Object3d> beamCore1;
	std::unique_ptr<Graphics::Object3d> beamCore2;
	std::unique_ptr<Graphics::Object3d> beamCore3;
	std::unique_ptr<Graphics::Object3d> beam1;
	std::unique_ptr<Graphics::Object3d> beam2;
	std::unique_ptr<Graphics::Object3d> beam3;
	std::unique_ptr<AABBCollider> collider;
	std::unique_ptr<Graphics::Object3d> model;
	std::unique_ptr<Graphics::Object3d> particleParent;
};

class Boss;

/// <summary>
/// BossItem:Beamクラス
/// </summary>
class Beam : public OriginGameObject {
public:
	Beam();
	~Beam();

	void Initialize()override;
	void Update()override;
	void Draw(Graphics::Material* mate = nullptr, bool is = false)override;
	void DebugGUI()override;
	void ParameterGUI();

	/// <summary>値の初期化</summary>
	void InitParameter();

	/// <summary>ビームの初期化</summary>
	void InitBeam(const Math::Vector3& pos, const Math::Vector3& velo);
	/// <summary>ビームの回転</summary>
	bool BeamRotate();

	/// <summary>ビームの更新</summary>
	bool BeamAttackUpdate();

	//========================================================================*/
	//* Collision
	void OnCollisionEnter([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionStay([[maybe_unused]] const ColliderInfo& other);
	void OnCollisionExit([[maybe_unused]] const ColliderInfo& other);

	//========================================================================*/
	//* Getter
	BaseCollider* GetCollider() { return beams_[0].collider.get(); }
	std::vector<OneBeam>& GetBeams() { return beams_; }
	float GetLifeTime() { return lifeTime_; }
	bool GetIsLive() { return isLive_; }
	BeamStep GetStep() { return step_; }
	float GetChangeTime() { return changeTime_; }

	//========================================================================*/
	//* Setter
	void SetIsLive(bool is);
	void SetBossParent(Boss* boss);

private:

	void ChangeBeamStep();
	void BeamExpand(BeamStep step);
	void BeamMove(BeamStep step);
	void BeamShrink(BeamStep step);

private:

	BeamStep step_ = BeamStep::AroundAttack;
	std::vector<OneBeam> beams_;

	std::unique_ptr<Graphics::Object3d> beamCore1_;
	std::unique_ptr<Graphics::Object3d> beamCore2_;
	std::unique_ptr<Graphics::Object3d> beamCore3_;
	std::unique_ptr<Graphics::Object3d> beam1_;
	std::unique_ptr<Graphics::Object3d> beam2_;
	std::unique_ptr<Graphics::Object3d> beam3_;
	std::unique_ptr<Graphics::Object3d> particleParent_;

	bool isLive_ = false;
	float lifeTime_ = 780.0f;

	float expandTime_ = 60.0f;
	float beamAttackTime_ = 240.0f;
	float shrinkTime_ = 60.0f;

	float changeTime_ = 0.0f;
	Math::Vector3 prePos_;
	Math::Vector3 targetPos_;

	float uvTransX_;

	float beam1BaseScale_;
	float beam2BaseScale_;
	float beam3BaseScale_;

	//std::unique_ptr<AABBCollider> collider_;

	// emitter
	ParticleEmitter spark1_;
	ParticleEmitter spark2_;
	ParticleEmitter* beamParticle_;
	ParticleEmitter* beamLight_;

};
