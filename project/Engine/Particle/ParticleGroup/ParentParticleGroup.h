#pragma once
#include "IParticleGroup.h"

class ParentParticleGroup : public IParticleGroup {
public:
	ParentParticleGroup();
	~ParentParticleGroup();

	void Update(const Math::Matrix4x4& billboardMatrix, Camera* camera) override;

	std::unique_ptr<ParticleEmitter> emitter_;

	BlendType type_ = BlendType::ADD;
};
