#pragma once
#include "IParticleGroup.h"

class ParticleGroup : public IParticleGroup {
public:
	ParticleGroup();
	~ParticleGroup();

	void Update(const Matrix4x4& billboardMatrix, Camera* camera) override;

	ParticleEmitter emitter_;
	BlendType type_ = BlendType::ADD;
	bool isSubMode_ = false;
};
