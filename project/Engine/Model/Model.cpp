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

void Model::Draw(ID3D12GraphicsCommandList* commandList)
{
	for (uint32_t index = 0; index < mesh_.size(); ++index) {
		commandList->SetGraphicsRootConstantBufferView(0, material_[index].GetMaterialResource()->GetGPUVirtualAddress());
		commandList->SetGraphicsRootDescriptorTable(2, material_[index].GetTexture()->gpuHandle);
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
