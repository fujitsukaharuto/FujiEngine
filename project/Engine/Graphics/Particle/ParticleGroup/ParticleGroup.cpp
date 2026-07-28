#include "ParticleGroup.h"
#include "Engine/DXC/FrameCount.h"

using namespace Math;
using namespace Graphics;


ParticleGroup::ParticleGroup() {
}

ParticleGroup::~ParticleGroup() {
	if (instancing_) {
		for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
			instancing_[i]->Unmap(0, nullptr);
			instancingDataGPU_[i] = nullptr;
		}
	}
}

void ParticleGroup::Update(const Matrix4x4& billboardMatrix, Camera* camera,uint32_t frameIndex) {
	int particleCount = 0;
	drawCount_ = 0;
	for (auto& particle : particles_) {
		if (LifeUpdate(particle)) {
			continue;
		}

		ParticleSizeUpdate(particle);

		Matrix4x4 worldViewProjectionMatrix;
		Matrix4x4 worldMatrix = MakeIdentity4x4();
		SRTUpdate(particle);
		Billboard(particle, worldMatrix, billboardMatrix, MakeIdentity4x4(), camera);

		if (camera) {
			const Matrix4x4& viewProjectionMatrix = camera->GetViewProjectionMatrix();
			worldViewProjectionMatrix = Multiply(worldMatrix, viewProjectionMatrix);
		} else {
			worldViewProjectionMatrix = worldMatrix;
		}

		instancingDataGPU_[frameIndex][particleCount].World = worldMatrix;
		instancingDataGPU_[frameIndex][particleCount].WVP = worldViewProjectionMatrix;
		instancingDataGPU_[frameIndex][particleCount].color = particle.color_;
		instancingDataGPU_[frameIndex][particleCount].uvTrans = particle.uvTrans_;
		instancingDataGPU_[frameIndex][particleCount].uvScale = particle.uvScale_;

		particleCount++;
		drawCount_++;
	}
}
