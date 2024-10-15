#include "ParticleManager.h"
#include "DXCom.h"
#include "SRVManager.h"
#include "CameraManager.h"
#include "Particle.h"




ParticleManager::ParticleManager() {
}

ParticleManager::~ParticleManager() {
}

ParticleManager* ParticleManager::GetInstance() {
	static ParticleManager instance;
	return &instance;
}

void ParticleManager::Initialize(DXCom* dxcom, SRVManager* srvManager) {

	dxCommon_ = dxcom;
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


	vBuffer_ = DXCom::GetInstance()->CreateBufferResource(DXCom::GetInstance()->GetDevice(), sizeof(VertexDate) * vertex_.size());
	iBuffer_ = DXCom::GetInstance()->CreateBufferResource(DXCom::GetInstance()->GetDevice(), sizeof(uint32_t) * index_.size());

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
	for (auto& groupPair : particleGroups_) {

		delete groupPair.second;

	}
	particleGroups_.clear();
}

void ParticleManager::Update() {


	for (auto& groupPair : particleGroups_) {

		ParticleGroup* group = groupPair.second;

		int particleCount = 0;
		float addpos = 1.5f;
		for (auto& particle : group->particles_) {

			if (particle.lifeTime_ <= 0) {
				particle.isLive_ = false;
				continue;
			}

			particle.transform.translate.y = addpos;

			addpos += 0.05f;

			Matrix4x4 worldMatrix = MakeAffineMatrix(particle.transform.scale, particle.transform.rotate, particle.transform.translate);
			Matrix4x4 worldViewProjectionMatrix;

			if (camera_) {
				const Matrix4x4& viewProjectionMatrix = camera_->GetViewProjectionMatrix();
				worldViewProjectionMatrix = Multiply(worldMatrix, viewProjectionMatrix);
			}
			else {
				worldViewProjectionMatrix = worldMatrix;
			}

			group->instancingData_[particleCount].World = worldMatrix;
			group->instancingData_[particleCount].WVP = worldViewProjectionMatrix;

			particleCount++;
		}
	}

}

void ParticleManager::Draw() {


	dxCommon_->GetDXCommand()->SetViewAndscissor();
	dxCommon_->GetPipelineManager()->SetPipeline(Pipe::particle);
	dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vbView);
	dxCommon_->GetCommandList()->IASetIndexBuffer(&ibView);

	for (auto& groupPair : particleGroups_) {
		ParticleGroup* group = groupPair.second;

		dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, group->material_.GetMaterialResource()->GetGPUVirtualAddress());
		dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(1, srvManager_->GetGPUDescriptorHandle(group->srvIndex_));
		dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, group->material_.GetTexture()->gpuHandle);

		dxCommon_->GetCommandList()->DrawIndexedInstanced(6, group->insstanceCount_, 0, 0, 0);
	}

}

void ParticleManager::CreateParticleGroup(const std::string name, const std::string fileName) {

	ParticleManager* instance = GetInstance();

	auto iterator = instance->particleGroups_.find(name);
	if (iterator != instance->particleGroups_.end()) {
		return;
	}


	ParticleGroup* newGroup = new ParticleGroup();
	newGroup->material_.SetTextureNamePath(fileName);
	newGroup->material_.CreateMaterial();


	newGroup->insstanceCount_ = 20;
	newGroup->instancing_ = instance->dxCommon_->CreateBufferResource(instance->dxCommon_->GetDevice(), (sizeof(TransformationParticleMatrix) * newGroup->insstanceCount_));
	newGroup->instancing_->Map(0, nullptr, reinterpret_cast<void**>(&newGroup->instancingData_));
	uint32_t max = newGroup->insstanceCount_;
	for (uint32_t index = 0; index < max; ++index) {
		newGroup->instancingData_[index].WVP = MakeIdentity4x4();
		newGroup->instancingData_[index].World = MakeIdentity4x4();
	}

	newGroup->srvIndex_ = instance->srvManager_->Allocate();
	instance->srvManager_->CreateStructuredSRV(newGroup->srvIndex_, newGroup->instancing_.Get(), newGroup->insstanceCount_, sizeof(TransformationParticleMatrix));


	//ここでパーティクルをあらかじめ作る
	float add = 0.1f;
	for (int i = 0; i < int(max); i++) {
		Particle p{};
		p.transform.scale = { 1.0f,1.0f,1.0f };
		p.transform.translate.x += add;
		p.transform.translate.y += add;
		newGroup->particles_.push_back(p);
		add += 0.1f;
	}


	instance->particleGroups_.insert(std::make_pair(name, newGroup));

}
