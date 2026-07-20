#pragma once
#include "IParticleGroup.h"

namespace Graphics {

	class ParentParticleGroup : public IParticleGroup {
	public:
		ParentParticleGroup();
		~ParentParticleGroup();

		void Update(const Math::Matrix4x4& billboardMatrix, Graphics::Camera* camera, uint32_t frameIndex) override;

		std::unique_ptr<ParticleEmitter> emitter_;

		Graphics::BlendType type_ = Graphics::BlendType::ADD;
	};

}
