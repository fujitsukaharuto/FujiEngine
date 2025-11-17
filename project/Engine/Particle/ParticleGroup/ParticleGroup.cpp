#include "ParticleGroup.h"

ParticleGroup::ParticleGroup() {
}

ParticleGroup::~ParticleGroup() {
	if (instancing_) {
		instancing_->Unmap(0, nullptr);
		instancingData_ = nullptr;
	}
}

void ParticleGroup::Update(const Matrix4x4& billboardMatrix, Camera* camera) {
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

		instancingData_[particleCount].World = worldMatrix;
		instancingData_[particleCount].WVP = worldViewProjectionMatrix;
		instancingData_[particleCount].color = particle.color_;
		instancingData_[particleCount].uvTrans = particle.uvTrans_;
		instancingData_[particleCount].uvScale = particle.uvScale_;

		particleCount++;
		drawCount_++;
	}
}
