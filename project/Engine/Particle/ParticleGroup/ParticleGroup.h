#pragma once
#include "IParticleGroup.h"

class ParticleGroup : public IParticleGroup {
public:
	ParticleGroup();
	~ParticleGroup();

	void Update(const Math::Matrix4x4& billboardMatrix, Camera* camera, uint32_t frameIndex) override;

	ParticleEmitter emitter_;
	BlendType type_ = BlendType::ADD;
	bool isSubMode_ = false;
};
