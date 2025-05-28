#include "Model.h"




Model::Model() {
	data_.rootNode.local = MakeIdentity4x4();
}

Model::~Model() {
	material_.clear();
	mesh_.clear();
}

void Model::Draw(ID3D12GraphicsCommandList* commandList, Material* mate = nullptr) {
	for (uint32_t index = 0; index < mesh_.size(); ++index) {
		if (mate) {
			commandList->SetGraphicsRootConstantBufferView(0, mate->GetMaterialResource()->GetGPUVirtualAddress());
			commandList->SetGraphicsRootDescriptorTable(2, mate->GetTexture()->gpuHandle);
		} else {
			commandList->SetGraphicsRootConstantBufferView(0, material_[index].GetMaterialResource()->GetGPUVirtualAddress());
			commandList->SetGraphicsRootDescriptorTable(2, material_[index].GetTexture()->gpuHandle);
		}
		mesh_[index].Draw(commandList);
	}
}

void Model::AnimationDraw(const SkinCluster& skinCluster, ID3D12GraphicsCommandList* commandList, Material* mate) {
	for (uint32_t index = 0; index < mesh_.size(); ++index) {
		if (mate) {
			commandList->SetGraphicsRootConstantBufferView(0, mate->GetMaterialResource()->GetGPUVirtualAddress());
			commandList->SetGraphicsRootDescriptorTable(2, mate->GetTexture()->gpuHandle);
		} else {
			commandList->SetGraphicsRootConstantBufferView(0, material_[index].GetMaterialResource()->GetGPUVirtualAddress());
			commandList->SetGraphicsRootDescriptorTable(2, material_[index].GetTexture()->gpuHandle);
		}
		mesh_[index].AnimationDraw(skinCluster,commandList);
	}
}

void Model::TransBarrier() {
	for (uint32_t index = 0; index < mesh_.size(); ++index) {
		mesh_[index].TransBarrier();
	}
}

void Model::AddMaterial(const Material& material) {
	material_.push_back(material);
}

void Model::AddMesh(const Mesh& mesh) {
	mesh_.push_back(mesh);
}

void Model::CreateEnvironment() {
	for (Material& material : material_) {
		material.CreateEnvironmentMaterial();
	}
	for (uint32_t index = 0; index < mesh_.size(); ++index) {
		mesh_[index].CreateUAV();
	}
}

void Model::CreateSkinningInformation(DXCom* pDxcom) {
	skinningInformation_ = pDxcom->CreateBufferResource(pDxcom->GetDevice(), sizeof(SkinningInformation));
	infoData_ = nullptr;
	skinningInformation_->Map(0, nullptr, reinterpret_cast<void**>(&infoData_));
	infoData_->numVertices = static_cast<int32_t>(mesh_.front().GetVertexDataSize());
}

void Model::SetColor(const Vector4& color) {
	for (Material& material : material_) {
		material.SetColor(color);
	}
}

void Model::SetUVScale(const Vector2& scale, const Vector2& uvTrans) {
	for (Material& material : material_) {
		material.SetUVScale(scale, uvTrans);
	}
}

void Model::SetAlphaRef(float ref) {
	for (Material& material : material_) {
		material.SetAlphaRef(ref);
	}
}

void Model::SetTexture(const std::string& name) {
	if (nowTextuer == name) {
		return;
	}
	for (uint32_t index = 0; index < mesh_.size(); ++index) {
		material_[index].SetTexture(name);
	}
	nowTextuer = name;
}

void Model::SetTextureName(const std::string& name) {
	nowTextuer = name;
}

void Model::SetLightEnable(LightMode mode) {
	for (uint32_t index = 0; index < mesh_.size(); ++index) {
		material_[index].SetLightEnable(mode);
	}
}

void Model::CSDispatch(const SkinCluster& skinCluster, ID3D12GraphicsCommandList* commandList) {
	PipelineManager::GetInstance()->SetCSPipeline(Pipe::SkinningCS);
	commandList->SetComputeRootDescriptorTable(0, skinCluster.paletteSrvHandle.second);
	commandList->SetComputeRootDescriptorTable(2, skinCluster.influenceSrvHandle.second);
	mesh_.front().CSDispatch(commandList);
	commandList->SetComputeRootConstantBufferView(4, skinningInformation_->GetGPUVirtualAddress());
	commandList->Dispatch(static_cast<UINT>(mesh_.front().GetVertexDataSize() + 1023) / 1024, 1, 1);
}
