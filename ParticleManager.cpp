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

	vertex_.push_back({ {-1.0f,-1.0f,0.0f,1.0f},{0.0f,0.0f},{0.0f,0.0f,-1.0f} });
	vertex_.push_back({ {-1.0f,1.0f,0.0f,1.0f},{0.0f,1.0f},{0.0f,0.0f,-1.0f} });
	vertex_.push_back({ {1.0f,1.0f,0.0f,1.0f},{1.0f,1.0f},{0.0f,0.0f,-1.0f} });
	vertex_.push_back({ {1.0f,-1.0f,0.0f,1.0f},{1.0f,0.0f},{0.0f,0.0f,-1.0f} });

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

void ParticleManager::Update() {


	for (auto& group : particleGroups_) {

		int particleCount = 0;
		float addpos = 0.01f;
		for (auto& particle : group.second.particles_) {

			if (particle.lifeTime_ <= 0) {
				particle.isLive_ = false;
			}

			particle.transform.translate.x += addpos;
			particle.transform.translate.y += addpos;

			addpos += 0.002f;

			Matrix4x4 worldMatrix = MakeAffineMatrix(particle.transform.scale, particle.transform.rotate, particle.transform.translate);
			Matrix4x4 worldViewProjectionMatrix;

			if (camera_) {
				const Matrix4x4& viewProjectionMatrix = camera_->GetViewProjectionMatrix();
				worldViewProjectionMatrix = Multiply(worldMatrix, viewProjectionMatrix);
			}
			else {
				worldViewProjectionMatrix = worldMatrix;
			}

			group.second.instancingData_[particleCount].World = worldMatrix;
			group.second.instancingData_[particleCount].WVP = worldViewProjectionMatrix;

		}
	}

}

void ParticleManager::Draw() {


	dxCommon_->GetDXCommand()->SetViewAndscissor();
	dxCommon_->GetPipelineManager()->SetPipeline(Pipe::particle);
	dxCommon_->GetDXCommand()->GetList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vbView);
	dxCommon_->GetCommandList()->IASetIndexBuffer(&ibView);

	for (auto& group : particleGroups_) {
		dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, group.second.material_->GetMaterialResource()->GetGPUVirtualAddress());
		dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(1, srvManager_->GetGPUDescriptorHandle(group.second.srvIndex_));
		dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, group.second.material_->GetTexture()->gpuHandle);

		dxCommon_->GetCommandList()->DrawIndexedInstanced(6, group.second.insstanceCount_, 0, 0, 0);
	}

}

void ParticleManager::CreateParticleGroup(const std::string name, const std::string fileName) {

	ParticleManager* instance = GetInstance();

	auto iterator = instance->particleGroups_.find(name);
	if (iterator != instance->particleGroups_.end()) {
		return;
	}


	ParticleGroup newGroup;
	newGroup.material_ = new Material();
	newGroup.material_->SetTextureNamePath(fileName);
	newGroup.material_->CreateMaterial();


	newGroup.insstanceCount_ = 20;
	newGroup.instancing_ = instance->dxCommon_->CreateBufferResource(instance->dxCommon_->GetDevice(), sizeof(TransformationParticleMatrix) * newGroup.insstanceCount_);


	newGroup.srvIndex_ = instance->srvManager_->Allocate();
	instance->srvManager_->CreateStructuredSRV(newGroup.srvIndex_, newGroup.instancing_.Get(), newGroup.insstanceCount_, sizeof(TransformationParticleMatrix));

	int max= newGroup.insstanceCount_;

	//ここでパーティクルをあらかじめ作る
	for (int i = 0; i < max; i++) {
		Particle p{};
		newGroup.particles_.push_back(p);
	}


	instance->particleGroups_.insert(std::make_pair(name, newGroup));

}
