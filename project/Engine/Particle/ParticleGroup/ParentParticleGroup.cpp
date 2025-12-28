#include "ParentParticleGroup.h"

using namespace Math;


ParentParticleGroup::ParentParticleGroup() {
}

ParentParticleGroup::~ParentParticleGroup() {
	if (instancing_) {
		for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
			instancing_[i]->Unmap(0, nullptr);
			instancingDataGPU_[i] = nullptr;
		}
	}
}

void ParentParticleGroup::Update(const Matrix4x4& billboardMatrix, Camera* camera, uint32_t frameIndex) {
	for (auto& particle : particles_) {
		if (!particle.isLive_) {
			continue;
		}
		if (!emitter_->GetIsUpdatedMatrix()) {
			emitter_->worldMatrix_ = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, emitter_->pos_);
			if (emitter_->HaveParent()) {
				const Matrix4x4& parentWorldMatrix = emitter_->GetParentMatrix();
				// スケール成分を除去した親ワールド行列を作成
				Matrix4x4 noScaleParentMatrix = parentWorldMatrix;

				// 各軸ベクトルの長さ（スケール）を計算
				Vector3 xAxis = { parentWorldMatrix.m[0][0], parentWorldMatrix.m[1][0], parentWorldMatrix.m[2][0] };
				Vector3 yAxis = { parentWorldMatrix.m[0][1], parentWorldMatrix.m[1][1], parentWorldMatrix.m[2][1] };
				Vector3 zAxis = { parentWorldMatrix.m[0][2], parentWorldMatrix.m[1][2], parentWorldMatrix.m[2][2] };

				float xLen = Vector3::Length(xAxis);
				float yLen = Vector3::Length(yAxis);
				float zLen = Vector3::Length(zAxis);

				// 正規化（スケールを除去）
				for (int i = 0; i < 3; ++i) {
					noScaleParentMatrix.m[i][0] /= xLen;
					noScaleParentMatrix.m[i][1] /= yLen;
					noScaleParentMatrix.m[i][2] /= zLen;
				}

				// 変換はそのまま（位置は影響受けてOKなら）
				emitter_->worldMatrix_ = Multiply(emitter_->worldMatrix_, noScaleParentMatrix);
			}
		}
		break;
	}

	int particleCount = 0;
	drawCount_ = 0;
	for (auto& particle : particles_) {
		if (LifeUpdate(particle)) {
			continue;
		}

		ParticleSizeUpdate(particle);

		Matrix4x4 worldViewProjectionMatrix;
		Matrix4x4 worldMatrix = MakeIdentity4x4();
		Matrix4x4 parentRotate = MakeIdentity4x4();
		SRTUpdate(particle);
		if (particle.isParentRotate_) {
			parentRotate = emitter_->worldMatrix_;
			// スケールを除去する（上のコードと同様）
			Vector3 xAxis = { parentRotate.m[0][0], parentRotate.m[1][0], parentRotate.m[2][0] };
			Vector3 yAxis = { parentRotate.m[0][1], parentRotate.m[1][1], parentRotate.m[2][1] };
			Vector3 zAxis = { parentRotate.m[0][2], parentRotate.m[1][2], parentRotate.m[2][2] };

			float xLen = Vector3::Length(xAxis);
			float yLen = Vector3::Length(yAxis);
			float zLen = Vector3::Length(zAxis);

			for (int i = 0; i < 3; ++i) {
				parentRotate.m[i][0] /= xLen;
				parentRotate.m[i][1] /= yLen;
				parentRotate.m[i][2] /= zLen;
			}
		}
		Billboard(particle, worldMatrix, billboardMatrix, parentRotate, camera);

		if (particle.isParent_) {
			// 親行列のスケール・回転を取り除いた「平行移動のみマトリクス」を作る
			Vector3 parentTranslate = { emitter_->worldMatrix_.m[3][0],
										emitter_->worldMatrix_.m[3][1],
										emitter_->worldMatrix_.m[3][2] };
			Matrix4x4 parentTranslateMatrix = MakeTranslateMatrix(parentTranslate);

			worldMatrix = Multiply(worldMatrix, parentTranslateMatrix);
		}
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
	emitter_->SetIsUpdatedMatrix(false);
}
