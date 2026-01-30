#pragma once
#include <wrl/client.h>
#include <map>
#include "Particle/Particle.h"
#include "Particle/ParticleEmitter.h"
#include "Math/Matrix/MatrixCalculation.h"
#include "Engine/DX/FrameCount.h"

using Microsoft::WRL::ComPtr;

class Camera;

class IParticleGroup {
public:
	IParticleGroup() = default;
	virtual ~IParticleGroup() = default;

	virtual void Update(const Math::Matrix4x4& billboardMatrix,Camera* camera, uint32_t frameIndex) = 0;

	Graphics::Material material_;
	std::list<Particle> particles_;
	uint32_t srvIndex_[DXC::kFrameCount_];
	ComPtr<ID3D12Resource> instancing_[DXC::kFrameCount_];
	uint32_t instanceCount_;
	TransformationParticleMatrix* instancingDataGPU_[DXC::kFrameCount_];
	uint32_t drawCount_;
	ShapeType shapeType_ = ShapeType::PLANE;

	/// <summary>パーティクルの寿命の処理</summary>
	bool LifeUpdate(Particle& particle);
	/// <summary>パーティクルのサイズのアップデート</summary>
	void ParticleSizeUpdate(Particle& particle);
	/// <summary>パーティクルのSRTのアップデート</summary>
	void SRTUpdate(Particle& particle);
	/// <summary>パーティクルのビルボード処理</summary>
	void Billboard(Particle& particle, Math::Matrix4x4& worldMatrix, const Math::Matrix4x4& billboardMatrix, const Math::Matrix4x4& rotate, Camera* camera);
	bool InitEmitParticle(Particle& particle, const Math::Vector3& pos, const Math::Vector3& rotate, const Particle& grain, const RandomParameter& para);

};