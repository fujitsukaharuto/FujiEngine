#include "ParticleManager.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/SRVManager.h"
#include "Engine/Camera/CameraManager.h"
#include "Particle.h"
#include "Math/Random/Random.h"
#include "Engine/DX/FPSKeeper.h"
#include "ImGuiManager/ImGuiManager.h"


ParticleManager::ParticleManager() {
}

ParticleManager::~ParticleManager() {
}

ParticleManager* ParticleManager::GetInstance() {
	static ParticleManager instance;
	return &instance;
}

void ParticleManager::Initialize(DXCom* pDxcom, SRVManager* srvManager) {
	dxcommon_ = pDxcom;
	srvManager_ = srvManager;
	this->camera_ = CameraManager::GetInstance()->GetCamera();

	vertex_.push_back({ {-1.0f,1.0f,0.0f,1.0f},{0.0f,0.0f},{0.0f,0.0f,-1.0f} });
	vertex_.push_back({ {-1.0f,-1.0f,0.0f,1.0f},{0.0f,1.0f},{0.0f,0.0f,-1.0f} });
	vertex_.push_back({ {1.0f,-1.0f,0.0f,1.0f},{1.0f,1.0f},{0.0f,0.0f,-1.0f} });
	vertex_.push_back({ {1.0f,1.0f,0.0f,1.0f},{1.0f,0.0f},{0.0f,0.0f,-1.0f} });

	index_.push_back(0);
	index_.push_back(3);
	index_.push_back(1);

	index_.push_back(1);
	index_.push_back(3);
	index_.push_back(2);


	vBuffer_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(VertexDate) * vertex_.size());
	iBuffer_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(uint32_t) * index_.size());

	VertexDate* vData = nullptr;
	vBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&vData));
	std::memcpy(vData, vertex_.data(), sizeof(VertexDate) * vertex_.size());

	vbView.BufferLocation = vBuffer_->GetGPUVirtualAddress();
	vbView.SizeInBytes = static_cast<UINT>(sizeof(VertexDate) * vertex_.size());
	vbView.StrideInBytes = static_cast<UINT>(sizeof(VertexDate));

	uint32_t* iData = nullptr;
	iBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&iData));
	std::memcpy(iData, index_.data(), sizeof(uint32_t) * index_.size());

	ibView.BufferLocation = iBuffer_->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R32_UINT;
	ibView.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * index_.size());
}

void ParticleManager::Finalize() {
#ifdef _DEBUG
	selectParticleGroup_ = nullptr;
#endif // _DEBUG

	dxcommon_ = nullptr;
	srvManager_ = nullptr;
	camera_ = nullptr;

	for (auto& group : particleGroups_) {
		if (group.second->instancing_) {
			group.second->instancing_->Unmap(0, nullptr);
			group.second->instancingData_ = nullptr;
		}
	}
	particleGroups_.clear();
	for (auto& group : parentParticleGroups_) {
		if (group.second->instancing_) {
			group.second->instancing_->Unmap(0, nullptr);
			group.second->instancingData_ = nullptr;
		}
	}
	parentParticleGroups_.clear();
	for (auto& groupPair : animeGroups_) {

		groupPair.second->lifeTime.clear();
		groupPair.second->startLifeTime_.clear();
		groupPair.second->isLive_.clear();
		groupPair.second->accele.clear();
		groupPair.second->speed.clear();
		groupPair.second.reset();
	}
	animeGroups_.clear();

	vBuffer_.Reset();
	iBuffer_.Reset();

}

void ParticleManager::Update() {
	Matrix4x4 billboardMatrix = MakeIdentity4x4();

	if (camera_) {
		const Matrix4x4& viewMatrix = camera_->GetViewMatrix();

		billboardMatrix.m[0][0] = viewMatrix.m[0][0];
		billboardMatrix.m[0][1] = viewMatrix.m[1][0];
		billboardMatrix.m[0][2] = viewMatrix.m[2][0];

		billboardMatrix.m[1][0] = viewMatrix.m[0][1];
		billboardMatrix.m[1][1] = viewMatrix.m[1][1];
		billboardMatrix.m[1][2] = viewMatrix.m[2][1];

		billboardMatrix.m[2][0] = viewMatrix.m[0][2];
		billboardMatrix.m[2][1] = viewMatrix.m[1][2];
		billboardMatrix.m[2][2] = viewMatrix.m[2][2];
	}


	for (auto& groupPair : particleGroups_) {

		ParticleGroup* group = groupPair.second.get();

#ifdef _DEBUG
		if (group->emitter_.isEmit_) {
			if (selectParticleGroup_ != group) {
				group->emitter_.Emit();
			}
		}
#endif // _DEBUG

		int particleCount = 0;
		group->drawCount_ = 0;
		for (auto& particle : group->particles_) {

			if (particle.lifeTime_ <= 0) {
				particle.isLive_ = false;
				continue;
			}

			particle.lifeTime_--;

			SizeType sizeType = SizeType(particle.type_);
			float t = (1.0f - float(float(particle.lifeTime_) / float(particle.startLifeTime_)));

			if (particle.isColorFade_) {
				particle.color_.w = Lerp(particle.startAlpha_, 0.0f, t * t);
			}

			switch (sizeType) {
			case SizeType::kNormal:
				break;
			case SizeType::kShift:

				particle.transform_.scale.x = Lerp(particle.startSize_.x, particle.endSize_.x, t);
				particle.transform_.scale.y = Lerp(particle.startSize_.y, particle.endSize_.y, t);

				break;
			case SizeType::kSin:

				Vector2 minSize = particle.startSize_; // 最小値
				Vector2 maxSize = particle.endSize_; // 最大値

				if (minSize.x > maxSize.x) {
					std::swap(minSize.x, maxSize.x); // minとmaxを交換
				}
				if (minSize.y > maxSize.y) {
					std::swap(minSize.y, maxSize.y); // minとmaxを交換
				}


				Vector2 sizeSin = minSize + (maxSize - minSize) * 0.5f * (1.0f + sin(particle.lifeTime_));

				particle.transform_.scale.x = sizeSin.x;
				particle.transform_.scale.y = sizeSin.y;

				break;
			}

			if (particle.rotateType_ == static_cast<int>(RotateType::kRandomR)) {
				if (particle.isContinuouslyRotate_) {
					particle.transform_.rotate += Random::GetVector3({ -0.2f,0.2f }, { -0.2f,0.2f }, { -0.2f,0.2f }) * FPSKeeper::DeltaTime();
				}
			}

			particle.speed_ += particle.accele_ * FPSKeeper::DeltaTime();

			particle.transform_.translate += particle.speed_ * FPSKeeper::DeltaTime();
			Matrix4x4 worldViewProjectionMatrix;
			Matrix4x4 worldMatrix = MakeIdentity4x4();

			if (!particle.isBillBoard_) {
				worldMatrix = MakeAffineMatrix(particle.transform_.scale, particle.transform_.rotate, particle.transform_.translate);
			}
			if (particle.isBillBoard_) {
				switch (particle.pattern_) {
				case BillBoardPattern::kXYZBillBoard:
					worldMatrix = Multiply(MakeScaleMatrix(particle.transform_.scale), billboardMatrix);
					worldMatrix = Multiply(worldMatrix, MakeTranslateMatrix(particle.transform_.translate));
					break;
				case BillBoardPattern::kXBillBoard: {

					Matrix4x4 xBillboardMatrix;
					xBillboardMatrix = billboardMatrix;
					xBillboardMatrix.m[1][0] = 0.0f; // Y軸成分をゼロにする
					xBillboardMatrix.m[2][0] = 0.0f; // Z軸成分をゼロにする

					worldMatrix = Multiply(MakeScaleMatrix(particle.transform_.scale), MakeRotateXYZMatrix({ 0.0f,particle.transform_.rotate.y,particle.transform_.rotate.z }));
					worldMatrix = Multiply(worldMatrix, xBillboardMatrix);
					worldMatrix = Multiply(worldMatrix, MakeTranslateMatrix(particle.transform_.translate));
					break;
				}
				case BillBoardPattern::kYBillBoard: {

					Matrix4x4 yBillboardMatrix = billboardMatrix;
					yBillboardMatrix.m[0][1] = 0.0f; // X軸成分をゼロにする
					yBillboardMatrix.m[2][1] = 0.0f; // Z軸成分をゼロにする

					worldMatrix = Multiply(MakeScaleMatrix(particle.transform_.scale), MakeRotateXYZMatrix({ particle.transform_.rotate.x,0.0f,particle.transform_.rotate.z }));
					worldMatrix = Multiply(worldMatrix, yBillboardMatrix);
					worldMatrix = Multiply(worldMatrix, MakeTranslateMatrix(particle.transform_.translate));

					break;
				}
				case BillBoardPattern::kZBillBoard: {

					Matrix4x4 zBillboardMatrix = billboardMatrix;
					zBillboardMatrix.m[0][2] = 0.0f; // X軸成分をゼロにする
					zBillboardMatrix.m[1][2] = 0.0f; // Y軸成分をゼロにする

					worldMatrix = Multiply(MakeScaleMatrix(particle.transform_.scale), MakeRotateXYZMatrix({ particle.transform_.rotate.x,particle.transform_.rotate.y,0.0f }));
					worldMatrix = Multiply(worldMatrix, zBillboardMatrix);
					worldMatrix = Multiply(worldMatrix, MakeTranslateMatrix(particle.transform_.translate));

					break;
				}
				case BillBoardPattern::kXYBillBoard: {
					Matrix4x4 xyBillboardMatrix = billboardMatrix;
					xyBillboardMatrix.m[0][1] = 0.0f; // X軸成分をゼロにする
					xyBillboardMatrix.m[2][1] = 0.0f; // Z軸成分をゼロにする
					xyBillboardMatrix.m[1][0] = 0.0f; // Y軸成分をゼロにする
					xyBillboardMatrix.m[2][0] = 0.0f; // Z軸成分をゼロにする

					worldMatrix = Multiply(MakeScaleMatrix(particle.transform_.scale), MakeRotateXYZMatrix({ particle.transform_.rotate.x,0.0f,particle.transform_.rotate.z }));
					worldMatrix = Multiply(worldMatrix, xyBillboardMatrix);
					worldMatrix = Multiply(worldMatrix, MakeTranslateMatrix(particle.transform_.translate));

					break;
				}
				default:
					break;
				}
			}

			if (camera_) {
				const Matrix4x4& viewProjectionMatrix = camera_->GetViewProjectionMatrix();
				worldViewProjectionMatrix = Multiply(worldMatrix, viewProjectionMatrix);
			} else {
				worldViewProjectionMatrix = worldMatrix;
			}

			group->instancingData_[particleCount].World = worldMatrix;
			group->instancingData_[particleCount].WVP = worldViewProjectionMatrix;
			group->instancingData_[particleCount].color = particle.color_;

			particleCount++;
			group->drawCount_++;
		}
	}


	for (auto& groupPair : parentParticleGroups_) {

		ParentParticleGroup* group = groupPair.second.get();

		for (auto& particle : group->particles_) {
			if (!particle.isLive_) {
				continue;
			}
			if (!group->emitter_->GetIsUpdatedMatrix()) {
				group->emitter_->worldMatrix_ = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, group->emitter_->pos_);
				if (group->emitter_->HaveParent()) {
					const Matrix4x4& parentWorldMatrix = group->emitter_->GetParentMatrix();
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
					group->emitter_->worldMatrix_ = Multiply(group->emitter_->worldMatrix_, noScaleParentMatrix);
				}
			}
			break;
		}

		int particleCount = 0;
		group->drawCount_ = 0;
		for (auto& particle : group->particles_) {

			if (particle.lifeTime_ <= 0) {
				particle.isLive_ = false;
				continue;
			}
			if (!particle.isLive_) {
				continue;
			}

			particle.lifeTime_--;

			SizeType sizeType = SizeType(particle.type_);
			float t = (1.0f - float(float(particle.lifeTime_) / float(particle.startLifeTime_)));

			if (particle.isColorFade_) {
				particle.color_.w = Lerp(particle.startAlpha_, 0.0f, t * t);
			}

			switch (sizeType) {
			case SizeType::kNormal:
				break;
			case SizeType::kShift:

				particle.transform_.scale.x = Lerp(particle.startSize_.x, particle.endSize_.x, t);
				particle.transform_.scale.y = Lerp(particle.startSize_.y, particle.endSize_.y, t);

				break;
			case SizeType::kSin:

				Vector2 minSize = particle.startSize_; // 最小値
				Vector2 maxSize = particle.endSize_; // 最大値

				if (minSize.x > maxSize.x) {
					std::swap(minSize.x, maxSize.x); // minとmaxを交換
				}
				if (minSize.y > maxSize.y) {
					std::swap(minSize.y, maxSize.y); // minとmaxを交換
				}


				Vector2 sizeSin = minSize + (maxSize - minSize) * 0.5f * (1.0f + sin(particle.lifeTime_));

				particle.transform_.scale.x = sizeSin.x;
				particle.transform_.scale.y = sizeSin.y;

				break;
			}

			if (particle.rotateType_ == static_cast<int>(RotateType::kRandomR)) {
				if (particle.isContinuouslyRotate_) {
					particle.transform_.rotate += Random::GetVector3({ -0.2f,0.2f }, { -0.2f,0.2f }, { -0.2f,0.2f }) * FPSKeeper::DeltaTime();
				}
			}

			particle.speed_ += particle.accele_ * FPSKeeper::DeltaTime();

			particle.transform_.translate += particle.speed_ * FPSKeeper::DeltaTime();
			Matrix4x4 worldViewProjectionMatrix;
			Matrix4x4 worldMatrix = MakeIdentity4x4();

			if (!particle.isBillBoard_) {
				worldMatrix = MakeAffineMatrix(particle.transform_.scale, particle.transform_.rotate, particle.transform_.translate);
			}
			if (particle.isBillBoard_) {
				switch (particle.pattern_) {
				case BillBoardPattern::kXYZBillBoard:
					worldMatrix = Multiply(MakeScaleMatrix(particle.transform_.scale), billboardMatrix);
					worldMatrix = Multiply(worldMatrix, MakeTranslateMatrix(particle.transform_.translate));
					break;
				case BillBoardPattern::kXBillBoard: {

					Matrix4x4 xBillboardMatrix;
					xBillboardMatrix = billboardMatrix;
					xBillboardMatrix.m[1][0] = 0.0f; // Y軸成分をゼロにする
					xBillboardMatrix.m[2][0] = 0.0f; // Z軸成分をゼロにする

					worldMatrix = Multiply(MakeScaleMatrix(particle.transform_.scale), MakeRotateXYZMatrix({ 0.0f,particle.transform_.rotate.y,particle.transform_.rotate.z }));
					worldMatrix = Multiply(worldMatrix, xBillboardMatrix);
					worldMatrix = Multiply(worldMatrix, MakeTranslateMatrix(particle.transform_.translate));
					break;
				}
				case BillBoardPattern::kYBillBoard: {
					if (camera_) {
						Matrix4x4 yBillboardMatrix = billboardMatrix;
						yBillboardMatrix.m[0][1] = 0.0f; // X軸成分をゼロにする
						yBillboardMatrix.m[2][1] = 0.0f; // Z軸成分をゼロにする

						worldMatrix = Multiply(MakeScaleMatrix(particle.transform_.scale), MakeRotateXYZMatrix({ particle.transform_.rotate.x,0.0f,particle.transform_.rotate.z }));
						worldMatrix = Multiply(worldMatrix, yBillboardMatrix);
						worldMatrix = Multiply(worldMatrix, MakeTranslateMatrix(particle.transform_.translate));

					}
					break;
				}
				case BillBoardPattern::kZBillBoard: {

					Matrix4x4 zBillboardMatrix = billboardMatrix;
					zBillboardMatrix.m[0][2] = 0.0f; // X軸成分をゼロにする
					zBillboardMatrix.m[1][2] = 0.0f; // Y軸成分をゼロにする

					worldMatrix = Multiply(MakeScaleMatrix(particle.transform_.scale), MakeRotateXYZMatrix({ particle.transform_.rotate.x,particle.transform_.rotate.y,0.0f }));
					worldMatrix = Multiply(worldMatrix, zBillboardMatrix);
					worldMatrix = Multiply(worldMatrix, MakeTranslateMatrix(particle.transform_.translate));

					break;
				}
				case BillBoardPattern::kXYBillBoard: {
					Matrix4x4 xyBillboardMatrix = billboardMatrix;
					xyBillboardMatrix.m[0][1] = 0.0f; // X軸成分をゼロにする
					xyBillboardMatrix.m[2][1] = 0.0f; // Z軸成分をゼロにする
					xyBillboardMatrix.m[1][0] = 0.0f; // Y軸成分をゼロにする
					xyBillboardMatrix.m[2][0] = 0.0f; // Z軸成分をゼロにする

					worldMatrix = Multiply(MakeScaleMatrix(particle.transform_.scale), MakeRotateXYZMatrix({ particle.transform_.rotate.x,0.0f,particle.transform_.rotate.z }));
					worldMatrix = Multiply(worldMatrix, xyBillboardMatrix);
					worldMatrix = Multiply(worldMatrix, MakeTranslateMatrix(particle.transform_.translate));

					break;
				}
				default:
					break;
				}
			}

			if (particle.isParent_) {
				// 親行列のスケール・回転を取り除いた「平行移動のみマトリクス」を作る
				Vector3 parentTranslate = { group->emitter_->worldMatrix_.m[3][0],
											group->emitter_->worldMatrix_.m[3][1],
											group->emitter_->worldMatrix_.m[3][2] };
				Matrix4x4 parentTranslateMatrix = MakeTranslateMatrix(parentTranslate);

				worldMatrix = Multiply(worldMatrix, parentTranslateMatrix);
			}
			if (camera_) {
				const Matrix4x4& viewProjectionMatrix = camera_->GetViewProjectionMatrix();
				worldViewProjectionMatrix = Multiply(worldMatrix, viewProjectionMatrix);
			} else {
				worldViewProjectionMatrix = worldMatrix;
			}

			group->instancingData_[particleCount].World = worldMatrix;
			group->instancingData_[particleCount].WVP = worldViewProjectionMatrix;
			group->instancingData_[particleCount].color = particle.color_;

			particleCount++;
			group->drawCount_++;
		}
		group->emitter_->SetIsUpdatedMatrix(false);
	}


	for (auto& groupPair : animeGroups_) {

		AnimeGroup* group = groupPair.second.get();
		for (int i = 0; i < group->objects_.size(); i++) {


			if (group->lifeTime[i] <= 0) {
				group->isLive_[i] = false;
				continue;
			}


			group->lifeTime[i] -= FPSKeeper::DeltaTime();
			group->animeTime[i] += FPSKeeper::DeltaTime();

			for (auto& animeChange : group->anime_) {
				if (group->animeTime[i] >= animeChange.second * FPSKeeper::DeltaTime()) {
					group->objects_[i]->SetTexture(animeChange.first);
				}
			}


			SizeType sizeType = SizeType(group->type);
			float t = (1.0f - float(float(group->lifeTime[i]) / float(group->startLifeTime_[i])));
			switch (sizeType) {
			case SizeType::kNormal:
				break;
			case SizeType::kShift:

				group->objects_[i]->transform.scale.x = Lerp(group->startSize.x, group->endSize.x, t);
				group->objects_[i]->transform.scale.y = Lerp(group->startSize.y, group->endSize.y, t);

				break;
			}

			group->speed[i] += group->accele[i] * FPSKeeper::DeltaTime();

			group->objects_[i]->transform.translate += group->speed[i] * FPSKeeper::DeltaTime();
			group->objects_[i]->SetBillboardMat(billboardMatrix);
		}
	}
}

void ParticleManager::Draw() {
	dxcommon_->GetDXCommand()->SetViewAndscissor();
	dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Normal);
	dxcommon_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (auto& groupPair : animeGroups_) {

		AnimeGroup* group = groupPair.second.get();
		for (int i = 0; i < group->objects_.size(); i++) {
			if (group->isLive_[i]) {
				group->objects_[i]->AnimeDraw();
			}

		}
	}


	dxcommon_->GetDXCommand()->SetViewAndscissor();
	dxcommon_->GetPipelineManager()->SetPipeline(Pipe::particle);
	dxcommon_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vbView);
	dxcommon_->GetCommandList()->IASetIndexBuffer(&ibView);

	for (auto& groupPair : particleGroups_) {
		ParticleGroup* group = groupPair.second.get();

		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(1, srvManager_->GetGPUDescriptorHandle(group->srvIndex_));
		dxcommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, group->material_.GetMaterialResource()->GetGPUVirtualAddress());
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, group->material_.GetTexture()->gpuHandle);

		dxcommon_->GetCommandList()->DrawIndexedInstanced(6, group->drawCount_, 0, 0, 0);
	}

	for (auto& groupPair : parentParticleGroups_) {
		ParentParticleGroup* group = groupPair.second.get();

		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(1, srvManager_->GetGPUDescriptorHandle(group->srvIndex_));
		dxcommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, group->material_.GetMaterialResource()->GetGPUVirtualAddress());
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, group->material_.GetTexture()->gpuHandle);

		dxcommon_->GetCommandList()->DrawIndexedInstanced(6, group->drawCount_, 0, 0, 0);
	}
}

void ParticleManager::ParticleDebugGUI() {
#ifdef _DEBUG
	ImGui::Begin("ParticleDebug");
	ImGui::SeparatorText("ParticleGroup");
	if (particleGroups_.size() != 0) {
		if (!selectParticleGroup_) {
			selectParticleGroup_ = particleGroups_.begin()->second.get();
		}
	}

	std::vector<const char*> keys;
	for (const auto& pair : particleGroups_) {
		keys.push_back(pair.first.c_str());
	}
	if (ImGui::Combo("Particle Group", &currentIndex_, keys.data(), int(keys.size()))) {
		currentKey_ = keys[currentIndex_];
		// currentKey を使って選択中の ParticleGroup を取得
		selectParticleGroup_ = particleGroups_[currentKey_].get();
	}

	ImGui::SeparatorText("SelectGroup");
	if (selectParticleGroup_) {
		ParticleEmitter& selecrtEmitter = selectParticleGroup_->emitter_;
		selecrtEmitter.DebugGUI();
	}

	ImGui::SeparatorText("Emit Control");
	if (ImGui::TreeNode("ParticleGroup Emit Control")) {
		static ParticleGroupSelector selector;
		if (ImGui::Button("ResetFrenquencyTime")) {
			for (auto& groupPair : particleGroups_) {
				groupPair.second->emitter_.TimeReset();
			}
		}

		// 初期化（1回だけ）
		if (selector.items[0].empty() && selector.items[1].empty()) {
			for (const auto& [name, group] : particleGroups_) {
				if (group->emitter_.isEmit_)
					selector.items[1].push_back(name);
				else
					selector.items[0].push_back(name);
			}
		}

		selector.Show([&](const std::string& name, bool emit) {
			auto it = particleGroups_.find(name);
			if (it != particleGroups_.end()) {
				it->second->emitter_.isEmit_ = emit;
			}
			});

		ImGui::TreePop();
	}

	ImGui::End();
#endif // _DEBUG
}

void ParticleManager::SelectParticleUpdate() {
#ifdef _DEBUG
	if (selectParticleGroup_) {
		selectParticleGroup_->emitter_.Emit();
	}
#endif // _DEBUG
}

void ParticleManager::SelectEmitterSizeDraw() {
#ifdef _DEBUG
	if (selectParticleGroup_) {
		selectParticleGroup_->emitter_.DrawSize();
	}
#endif // _DEBUG
}

void ParticleManager::CreateParticleGroup(const std::string& name, const std::string& fileName, uint32_t count) {
	ParticleManager* instance = GetInstance();

	auto iterator = instance->particleGroups_.find(name);
	if (iterator != instance->particleGroups_.end()) {
		return;
	}


	ParticleGroup* newGroup = new ParticleGroup();

	newGroup->emitter_.name_ = name;
	newGroup->emitter_.Load(name);

	newGroup->insstanceCount_ = count;
	newGroup->instancing_ = instance->dxcommon_->CreateBufferResource(instance->dxcommon_->GetDevice(), (sizeof(TransformationParticleMatrix) * newGroup->insstanceCount_));
	newGroup->instancing_->Map(0, nullptr, reinterpret_cast<void**>(&newGroup->instancingData_));
	uint32_t max = newGroup->insstanceCount_;
	for (uint32_t index = 0; index < max; ++index) {
		newGroup->instancingData_[index].WVP = MakeIdentity4x4();
		newGroup->instancingData_[index].World = MakeIdentity4x4();
	}
	newGroup->material_.SetTextureNamePath(fileName);
	newGroup->material_.CreateMaterial();
	newGroup->srvIndex_ = instance->srvManager_->Allocate();
	instance->srvManager_->CreateStructuredSRV(newGroup->srvIndex_, newGroup->instancing_.Get(), newGroup->insstanceCount_, sizeof(TransformationParticleMatrix));


	//ここでパーティクルをあらかじめ作る
	float add = 0.1f;
	for (int i = 0; i < int(max); i++) {
		Particle p{};
		p.transform_.scale = { 1.0f,1.0f,1.0f };
		p.transform_.translate.x += add;
		p.transform_.translate.y += add;
		newGroup->particles_.push_back(p);
		add += 0.1f;
	}


	instance->particleGroups_.insert(std::make_pair(name, newGroup));
}

void ParticleManager::CreateParentParticleGroup(const std::string& name, const std::string& fileName, uint32_t count) {

	ParticleManager* instance = GetInstance();

	auto iterator = instance->parentParticleGroups_.find(name);
	if (iterator != instance->parentParticleGroups_.end()) {
		return;
	}

	auto newGroup = std::make_unique<ParentParticleGroup>();

	// 必要な初期化処理
	newGroup->emitter_ = std::make_unique<ParticleEmitter>();
	newGroup->emitter_->name_ = name;
	newGroup->emitter_->Load(name);

	newGroup->insstanceCount_ = count;
	newGroup->instancing_ = instance->dxcommon_->CreateBufferResource(instance->dxcommon_->GetDevice(), (sizeof(TransformationParticleMatrix) * newGroup->insstanceCount_));
	newGroup->instancing_->Map(0, nullptr, reinterpret_cast<void**>(&newGroup->instancingData_));
	uint32_t max = newGroup->insstanceCount_;
	for (uint32_t index = 0; index < max; ++index) {
		newGroup->instancingData_[index].WVP = MakeIdentity4x4();
		newGroup->instancingData_[index].World = MakeIdentity4x4();
	}
	newGroup->material_.SetTextureNamePath(fileName);
	newGroup->material_.CreateMaterial();
	newGroup->srvIndex_ = instance->srvManager_->Allocate();
	instance->srvManager_->CreateStructuredSRV(newGroup->srvIndex_, newGroup->instancing_.Get(), newGroup->insstanceCount_, sizeof(TransformationParticleMatrix));


	//ここでパーティクルをあらかじめ作る
	float add = 0.1f;
	for (int i = 0; i < int(max); i++) {
		Particle p{};
		p.transform_.scale = { 1.0f,1.0f,1.0f };
		p.transform_.translate.x += add;
		p.transform_.translate.y += add;
		newGroup->particles_.push_back(p);
		add += 0.1f;
	}

	instance->parentParticleGroups_.emplace(name, std::move(newGroup));
}

void ParticleManager::CreateAnimeGroup(const std::string& name, const std::string& fileName) {
	ParticleManager* instance = GetInstance();

	auto iterator = instance->animeGroups_.find(name);
	if (iterator != instance->animeGroups_.end()) {
		return;
	}

	std::unique_ptr<AnimeGroup> newGroup;
	newGroup = std::make_unique<AnimeGroup>();

	newGroup->farst = fileName;
	TextureManager::GetInstance()->LoadTexture(fileName);
	for (int i = 0; i < 6; i++) {
		std::unique_ptr<Object3d> newobj = std::make_unique<Object3d>();
		newobj->Create("plane.obj");
		newobj->SetTexture(fileName);


		newGroup->objects_.push_back(std::move(newobj));
		newGroup->lifeTime.push_back(0.0f);
		newGroup->animeTime.push_back(0.0f);
		newGroup->startLifeTime_.push_back(0.0f);
		newGroup->isLive_.push_back(false);
		newGroup->accele.push_back({ 0.0f,0.0f,0.0f });
		newGroup->speed.push_back({ 0.0f,0.0f,0.0f });

	}

	instance->animeGroups_.insert(std::make_pair(name, std::move(newGroup)));
}

void ParticleManager::Load(ParticleEmitter& emit, const std::string& name) {
	ParticleManager* instance = GetInstance();
	emit.name_ = name;

	auto iterator = instance->particleGroups_.find(name);
	if (iterator != instance->particleGroups_.end()) {
		ParticleGroup* group = iterator->second.get();
		emit.pos_                 = group->emitter_.pos_;
		emit.particleRotate_      = group->emitter_.particleRotate_;
		emit.emitSizeMax_         = group->emitter_.emitSizeMax_;
		emit.emitSizeMin_         = group->emitter_.emitSizeMin_;
		emit.count_               = group->emitter_.count_;
		emit.frequencyTime_       = group->emitter_.frequencyTime_;
		emit.grain_.lifeTime_     = group->emitter_.grain_.lifeTime_;
		emit.grain_.accele_       = group->emitter_.grain_.accele_;
		emit.grain_.speed_        = group->emitter_.grain_.speed_;
		emit.grain_.type_         = group->emitter_.grain_.type_;
		emit.grain_.speedType_    = group->emitter_.grain_.speedType_;
		emit.grain_.rotateType_   = group->emitter_.grain_.rotateType_;
		emit.grain_.colorType_    = group->emitter_.grain_.colorType_;
		emit.grain_.returnPower_  = group->emitter_.grain_.returnPower_;
		emit.grain_.startSize_    = group->emitter_.grain_.startSize_;
		emit.grain_.endSize_      = group->emitter_.grain_.endSize_;
		emit.grain_.isBillBoard_  = group->emitter_.grain_.isBillBoard_;
		emit.grain_.pattern_      = group->emitter_.grain_.pattern_;
		emit.para_.speedx         = group->emitter_.para_.speedx;
		emit.para_.speedy         = group->emitter_.para_.speedy;
		emit.para_.speedz         = group->emitter_.para_.speedz;
		emit.para_.transx         = group->emitter_.para_.transx;
		emit.para_.transy         = group->emitter_.para_.transy;
		emit.para_.transz         = group->emitter_.para_.transz;
		emit.para_.colorMin       = group->emitter_.para_.colorMin;
		emit.para_.colorMax       = group->emitter_.para_.colorMax;

	} else {
		return;
	}
}

void ParticleManager::LoadParentGroup(ParticleEmitter*& emit, const std::string& name) {
	ParticleManager* instance = GetInstance();

	auto iterator = instance->parentParticleGroups_.find(name);
	if (iterator != instance->parentParticleGroups_.end()) {
		ParentParticleGroup* group = iterator->second.get();
		emit = group->emitter_.get();
		emit->grain_.isParent_ = true;

	} else {
		return;
	}
}

void ParticleManager::Emit(const std::string& name, const Vector3& pos, const Vector3& rotate, const Particle& grain, const RandomParametor& para, uint32_t count) {
	ParticleManager* instance = GetInstance();

	auto iterator = instance->particleGroups_.find(name);
	if (iterator != instance->particleGroups_.end()) {
		uint32_t newCount = 0;

		ParticleGroup* group = iterator->second.get();
		for (auto& particle : group->particles_) {

			if (particle.isLive_ == false) {
				particle.transform_ = grain.transform_;
				particle.transform_.translate = Random::GetVector3(para.transx, para.transy, para.transz);
				particle.transform_.translate += pos;
				particle.transform_.scale = { grain.startSize_.x,grain.startSize_.y,1.0f };
				if (grain.speedType_ == static_cast<int>(SpeedType::kCenter)) {
					particle.speed_ = grain.speed_;
				} else {
					particle.speed_ = Random::GetVector3(para.speedx, para.speedy, para.speedz);
				}
				particle.returnPower_ = grain.returnPower_;


				particle.rotateType_ = grain.rotateType_;
				particle.isContinuouslyRotate_ = grain.isContinuouslyRotate_;
				Vector3 veloSpeed = particle.speed_.Normalize();
				Vector3 cameraR{};
				Vector3 defo = { 0.0f,1.0f,0.0f };
				Vector3 angleDToD{};
				Matrix4x4 rotateCamera;
				Matrix4x4 dToD;

				switch (particle.rotateType_) {
				case static_cast<int>(RotateType::kUsually):
					particle.transform_.rotate = rotate;
					break;
				case static_cast<int>(RotateType::kVelocityR):

					veloSpeed = particle.speed_.Normalize();

					// カメラの回転を考慮して速度ベクトルを変換
					cameraR = CameraManager::GetInstance()->GetCamera()->transform.rotate;
					rotateCamera = MakeRotateXYZMatrix(-cameraR);
					veloSpeed = TransformNormal(veloSpeed, rotateCamera);

					defo = TransformNormal(defo, rotateCamera);

					dToD = DirectionToDirection(defo, veloSpeed.Normalize());
					angleDToD = ExtractEulerAngles(dToD);
					particle.transform_.rotate = angleDToD;

					break;
				case static_cast<int>(RotateType::kRandomR):
					particle.transform_.rotate = Random::GetVector3({ -1.0f,1.0f }, { -1.0f,1.0f }, { -1.0f,1.0f });
					break;
				}

				particle.lifeTime_ = grain.lifeTime_;
				particle.startLifeTime_ = particle.lifeTime_;
				particle.isBillBoard_ = grain.isBillBoard_;
				particle.pattern_ = grain.pattern_;
				particle.colorType_ = grain.colorType_;
				particle.isColorFade_ = grain.isColorFade_;
				switch (particle.colorType_) {
				case static_cast<int>(ColorType::kDefault):
					particle.color_ = para.colorMax;
					break;
				case static_cast<int>(ColorType::kRandom):
					particle.color_.x = Random::GetFloat(para.colorMin.x, para.colorMax.x);
					particle.color_.y = Random::GetFloat(para.colorMin.y, para.colorMax.y);
					particle.color_.z = Random::GetFloat(para.colorMin.z, para.colorMax.z);
					particle.color_.w = Random::GetFloat(para.colorMin.w, para.colorMax.w);
					break;
				}
				particle.startAlpha_ = particle.color_.w;

				SpeedType type = SpeedType(grain.speedType_);
				switch (type) {
				case SpeedType::kConstancy:
					particle.accele_ = Vector3{ 0.0f,0.0f,0.0f };
					break;
				case SpeedType::kChange:
					particle.accele_ = grain.accele_;
					break;
				case SpeedType::kReturn:
					particle.accele_ = (particle.speed_) * grain.returnPower_;
					break;
				case SpeedType::kCenter:
					particle.accele_ = Vector3{ 0.0f,0.0f,0.0f };
					break;
				}

				particle.type_ = grain.type_;
				particle.startSize_ = grain.startSize_;
				particle.endSize_ = grain.endSize_;

				particle.isLive_ = true;
				newCount++;
			}
			if (newCount == count) {
				return;
			}
		}
	} else {
		return;
	}
}

void ParticleManager::ParentEmit(const std::string& name, const Vector3& pos, const Vector3& rotate, const Particle& grain, const RandomParametor& para, uint32_t count) {
	ParticleManager* instance = GetInstance();

	auto iterator = instance->parentParticleGroups_.find(name);
	if (iterator != instance->parentParticleGroups_.end()) {
		uint32_t newCount = 0;

		ParentParticleGroup* group = iterator->second.get();
		for (auto& particle : group->particles_) {

			if (particle.isLive_ == false) {
				particle.transform_ = grain.transform_;
				particle.transform_.translate = Random::GetVector3(para.transx, para.transy, para.transz);
				particle.transform_.translate += pos;
				particle.transform_.scale = { grain.startSize_.x,grain.startSize_.y,1.0f };
				if (grain.speedType_ == static_cast<int>(SpeedType::kCenter)) {
					particle.speed_ = grain.speed_;
				} else {
					particle.speed_ = Random::GetVector3(para.speedx, para.speedy, para.speedz);
				}
				particle.returnPower_ = grain.returnPower_;


				particle.rotateType_ = grain.rotateType_;
				particle.isContinuouslyRotate_ = grain.isContinuouslyRotate_;
				Vector3 veloSpeed = particle.speed_.Normalize();
				Vector3 cameraR{};
				Vector3 defo = { 0.0f,1.0f,0.0f };
				Vector3 angleDToD{};
				Matrix4x4 rotateCamera;
				Matrix4x4 dToD;

				switch (particle.rotateType_) {
				case static_cast<int>(RotateType::kUsually):
					particle.transform_.rotate = rotate;
					break;
				case static_cast<int>(RotateType::kVelocityR):

					veloSpeed = particle.speed_.Normalize();

					// カメラの回転を考慮して速度ベクトルを変換
					cameraR = CameraManager::GetInstance()->GetCamera()->transform.rotate;
					rotateCamera = MakeRotateXYZMatrix(-cameraR);
					veloSpeed = TransformNormal(veloSpeed, rotateCamera);

					defo = TransformNormal(defo, rotateCamera);

					dToD = DirectionToDirection(defo, veloSpeed.Normalize());
					angleDToD = ExtractEulerAngles(dToD);
					particle.transform_.rotate = angleDToD;

					break;
				case static_cast<int>(RotateType::kRandomR):
					particle.transform_.rotate = Random::GetVector3({ -1.0f,1.0f }, { -1.0f,1.0f }, { -1.0f,1.0f });
					break;
				}

				particle.lifeTime_ = grain.lifeTime_;
				particle.startLifeTime_ = particle.lifeTime_;
				particle.isBillBoard_ = grain.isBillBoard_;
				particle.pattern_ = grain.pattern_;
				particle.colorType_ = grain.colorType_;
				particle.isColorFade_ = grain.isColorFade_;
				switch (particle.colorType_) {
				case static_cast<int>(ColorType::kDefault):
					particle.color_ = para.colorMax;
					break;
				case static_cast<int>(ColorType::kRandom):
					particle.color_.x = Random::GetFloat(para.colorMin.x, para.colorMax.x);
					particle.color_.y = Random::GetFloat(para.colorMin.y, para.colorMax.y);
					particle.color_.z = Random::GetFloat(para.colorMin.z, para.colorMax.z);
					particle.color_.w = Random::GetFloat(para.colorMin.w, para.colorMax.w);
					break;
				}
				particle.startAlpha_ = particle.color_.w;

				SpeedType type = SpeedType(grain.speedType_);
				switch (type) {
				case SpeedType::kConstancy:
					particle.accele_ = Vector3{ 0.0f,0.0f,0.0f };
					break;
				case SpeedType::kChange:
					particle.accele_ = grain.accele_;
					break;
				case SpeedType::kReturn:
					particle.accele_ = (particle.speed_) * grain.returnPower_;
					break;
				case SpeedType::kCenter:
					particle.accele_ = Vector3{ 0.0f,0.0f,0.0f };
					break;
				}

				particle.type_ = grain.type_;
				particle.startSize_ = grain.startSize_;
				particle.endSize_ = grain.endSize_;
				particle.isParent_ = grain.isParent_;

				particle.isLive_ = true;
				newCount++;
			}
			if (newCount == count) {
				return;
			}
		}
	} else {
		return;
	}
}

void ParticleManager::EmitAnime(const std::string& name, const Vector3& pos, const AnimeData& data, const RandomParametor& para, uint32_t count) {
	ParticleManager* instance = GetInstance();

	auto iterator = instance->animeGroups_.find(name);
	if (iterator != instance->animeGroups_.end()) {
		uint32_t newCount = 0;


		AnimeGroup* group = iterator->second.get();
		group->speedType = data.speedType;
		group->type = data.type;
		group->startSize = data.startSize;
		group->endSize = data.endSize;
		for (int i = 0; i < group->objects_.size(); i++) {

			if (!group->isLive_[i]) {
				group->objects_[i]->transform.translate = Random::GetVector3(para.transx, para.transy, para.transz);
				group->objects_[i]->transform.translate += pos;
				group->speed[i] = Random::GetVector3(para.speedx, para.speedy, para.speedz);
				group->lifeTime[i] = data.lifeTime;
				group->startLifeTime_[i] = group->lifeTime[i];
				group->animeTime[i] = 0.0f;

				SpeedType type = SpeedType(group->speedType);
				switch (type) {
				case SpeedType::kConstancy:
					group->accele[i] = Vector3{ 0.0f,0.0f,0.0f };
					break;
				case SpeedType::kChange:
					group->accele[i] = (group->speed[i]) * -0.05f;
					break;
				}

				group->objects_[i]->SetTexture(group->farst);
				group->isLive_[i] = true;
				newCount++;
			}
			if (newCount == count) {
				return;
			}
		}
	} else {
		return;
	}
}

void ParticleManager::AddAnime(const std::string& name, const std::string& fileName, float animeChangeTime) {
	ParticleManager* instance = GetInstance();

	auto iterator = instance->animeGroups_.find(name);
	if (iterator != instance->animeGroups_.end()) {

		AnimeGroup* group = iterator->second.get();
		TextureManager::GetInstance()->LoadTexture(fileName);
		group->anime_.insert(std::make_pair(fileName, animeChangeTime));

	} else {
		return;
	}
}