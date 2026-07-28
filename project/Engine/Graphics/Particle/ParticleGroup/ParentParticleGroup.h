#pragma once
#include "IParticleGroup.h"
#include "Engine/Graphics/Pipeline/BasePipeline.h"

namespace Graphics {

	/// <summary>
	/// 親オブジェクトに追従させるパーティクルグループ
	/// </summary>
	class ParentParticleGroup : public IParticleGroup {
	public:
		ParentParticleGroup();
		~ParentParticleGroup();

		void Update(const Math::Matrix4x4& billboardMatrix, Graphics::Camera* camera, uint32_t frameIndex) override;

		std::unique_ptr<ParticleEmitter> emitter_;

		Graphics::BlendType type_ = Graphics::BlendType::ADD;
	};

}
