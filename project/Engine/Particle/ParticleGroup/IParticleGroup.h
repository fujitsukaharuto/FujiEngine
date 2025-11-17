#pragma once
#include <wrl/client.h>
#include <map>
#include "Particle/Particle.h"
#include "Particle/ParticleEmitter.h"
#include "Math/Matrix/MatrixCalculation.h"

using Microsoft::WRL::ComPtr;

class Camera;

class IParticleGroup {
public:
	IParticleGroup() = default;
	virtual ~IParticleGroup() = default;

	virtual void Update(const Matrix4x4& billboardMatrix,Camera* camera) = 0;

	Material material_;
	std::list<Particle> particles_;
	uint32_t srvIndex_;
	ComPtr<ID3D12Resource> instancing_ = nullptr;
	uint32_t insstanceCount_;
	TransformationParticleMatrix* instancingData_ = nullptr;
	uint32_t drawCount_;
	ShapeType shapeType_ = ShapeType::PLANE;

	/// <summary>パーティクルの寿命の処理</summary>
	bool LifeUpdate(Particle& particle);
	/// <summary>パーティクルのサイズのアップデート</summary>
	void ParticleSizeUpdate(Particle& particle);
	/// <summary>パーティクルのSRTのアップデート</summary>
	void SRTUpdate(Particle& particle);
	/// <summary>パーティクルのビルボード処理</summary>
	void Billboard(Particle& particle, Matrix4x4& worldMatrix, const Matrix4x4& billboardMatrix, const Matrix4x4& rotate, Camera* camera);
	bool InitEmitParticle(Particle& particle, const Vector3& pos, const Vector3& rotate, const Particle& grain, const RandomParametor& para);
};