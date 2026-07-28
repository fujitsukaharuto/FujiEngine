#pragma once
#include <wrl/client.h>
#include <map>
#include "Engine/Graphics/Particle/Particle.h"
#include "Engine/Graphics/Particle/ParticleEmitter.h"
#include "Engine/Math/Matrix/MatrixCalculation.h"
#include "Engine/DXC/FrameCount.h"

using Microsoft::WRL::ComPtr;

namespace Graphics {

	class Camera;

	/// <summary>
	/// CPUパーティクルのグループの基底クラス
	/// </summary>
	class IParticleGroup {
	public:
		IParticleGroup() = default;
		virtual ~IParticleGroup() = default;

		virtual void Update(const Math::Matrix4x4& billboardMatrix,Graphics::Camera* camera, uint32_t frameIndex) = 0;

		/// <summary>パーティクルの寿命の処理</summary>
		bool LifeUpdate(Particle& particle);
		/// <summary>パーティクルのサイズのアップデート</summary>
		void ParticleSizeUpdate(Particle& particle);
		/// <summary>パーティクルのSRTのアップデート</summary>
		void SRTUpdate(Particle& particle);
		/// <summary>パーティクルのビルボード処理</summary>
		void Billboard(Particle& particle, Math::Matrix4x4& worldMatrix, const Math::Matrix4x4& billboardMatrix, const Math::Matrix4x4& rotate, Graphics::Camera* camera);
		bool InitEmitParticle(Particle& particle, const Math::Vector3& pos, const Math::Vector3& rotate, const Particle& grain, const RandomParameter& para);

		//========================================================================*/
		//* Getter

		Graphics::Material& GetMaterial() { return material_; }
		std::list<Particle>& GetParticles() { return particles_; }
		uint32_t& GetSRVIndex(UINT index) { return srvIndex_[index]; }
		uint32_t& GetInstanceCount() { return instanceCount_; }
		uint32_t& GetDrawCount() { return drawCount_; }
		Microsoft::WRL::ComPtr<ID3D12Resource>& GetInstancingResource(UINT index) { return instancing_[index]; }
		TransformationParticleMatrix*& GetInstancingDataGPU(UINT index) { return instancingDataGPU_[index]; }
		ShapeType& GetShapeType() { return shapeType_; }

	protected:
		Graphics::Material material_;
		std::list<Particle> particles_;
		uint32_t srvIndex_[DXC::kFrameCount_];
		Microsoft::WRL::ComPtr<ID3D12Resource> instancing_[DXC::kFrameCount_];
		uint32_t instanceCount_;
		TransformationParticleMatrix* instancingDataGPU_[DXC::kFrameCount_];
		uint32_t drawCount_;
		ShapeType shapeType_ = ShapeType::PLANE;

	};

}
