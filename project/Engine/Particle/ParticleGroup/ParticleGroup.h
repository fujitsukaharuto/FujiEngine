#pragma once
#include "IParticleGroup.h"

class ParticleGroup : public IParticleGroup {
public:
	ParticleGroup();
	~ParticleGroup();

	void Update(const Matrix4x4& billboardMatrix, Camera* camera) override;

	ParticleEmitter emitter_;
	bool isSubMode_ = false;
};
