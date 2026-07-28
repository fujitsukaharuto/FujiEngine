#pragma once
#include "IParticleGroup.h"
#include "Engine/Graphics/Pipeline/BasePipeline.h"

namespace Graphics {

	/// <summary>
	/// エミッターを1つ持つ通常のパーティクルグループ
	/// </summary>
	class ParticleGroup : public IParticleGroup {
	public:
		ParticleGroup();
		~ParticleGroup();

		void Update(const Math::Matrix4x4& billboardMatrix, Graphics::Camera* camera, uint32_t frameIndex) override;

		ParticleEmitter emitter_;
		Graphics::BlendType type_ = Graphics::BlendType::ADD;
		bool isSubMode_ = false;
	};

}
