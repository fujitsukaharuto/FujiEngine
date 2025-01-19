#include "Model.h"




Model::Model() {}
Model::Model(const Model& other) {

	mesh_ = other.mesh_;
	for (uint32_t index = 0; index < other.material_.size(); ++index) {
		Material newMaterial{};
		newMaterial.SetTextureNamePath(other.material_[index].GetPathName());
		newMaterial.CreateMaterial();
		material_.push_back(newMaterial);
	}

}
Model::~Model() {}

void Model::Draw(ID3D12GraphicsCommandList* commandList, Material* mate = nullptr)
{
	for (uint32_t index = 0; index < mesh_.size(); ++index) {
		if (mate) {
			commandList->SetGraphicsRootConstantBufferView(0, mate->GetMaterialResource()->GetGPUVirtualAddress());
			commandList->SetGraphicsRootDescriptorTable(2, mate->GetTexture()->gpuHandle);
		}
		else {
			commandList->SetGraphicsRootConstantBufferView(0, material_[index].GetMaterialResource()->GetGPUVirtualAddress());
			commandList->SetGraphicsRootDescriptorTable(2, material_[index].GetTexture()->gpuHandle);
		}
		mesh_[index].Draw(commandList);
	}
}


void Model::AddMaterial(const Material& material) {
	material_.push_back(material);
}


void Model::AddMesh(const Mesh& mesh) {
	mesh_.push_back(mesh);
}

void Model::SetColor(const Vector4& color) {
	for (Material material : material_) {
		material.SetColor(color);
	}
}

void Model::SetUVScale(const Vector2& scale, const Vector2& uvTrans) {
	for (Material material : material_) {
		material.SetUVScale(scale,uvTrans);
	}
}

void Model::SetTexture(const std::string& name) {

	if (nowTextuer==name) {
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

void Model::ShaderTextureDraw(ID3D12GraphicsCommandList* commandList) {
	for (uint32_t index = 0; index < mesh_.size(); ++index) {
		commandList->SetGraphicsRootConstantBufferView(0, material_[index].GetMaterialResource()->GetGPUVirtualAddress());
		commandList->SetGraphicsRootDescriptorTable(2, DXCom::GetInstance()->GetShaderTextureGPU());
		mesh_[index].Draw(commandList);
	}
}
