#include "Model.h"
#include "DXCom.h"

#include <fstream>
#include <sstream>


Model::Model() {}

Model::Model(const Model& other) {

	mesh_ = other.mesh_;
	material_ = other.material_;
	wvpResource_ = DXCom::GetInstance()->CreateBufferResource(DXCom::GetInstance()->GetDevice(), sizeof(TransformationMatrix));
	wvpDate_ = nullptr;
	wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpDate_));
	wvpDate_->WVP = MakeIdentity4x4();
	wvpDate_->World = MakeIdentity4x4();
}

Model::~Model() {}

void Model::Draw()
{

	ID3D12GraphicsCommandList* cList = DXCom::GetInstance()->GetCommandList();

	for (uint32_t index = 0; index < mesh_.size(); ++index) {
		cList->SetGraphicsRootConstantBufferView(0, material_[index].GetMaterialResource()->GetGPUVirtualAddress());
		cList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
		cList->SetGraphicsRootConstantBufferView(3, material_[index].GetDirectionLight()->GetGPUVirtualAddress());
		cList->SetGraphicsRootDescriptorTable(2, material_[index].GetTexture()->gpuHandle);
		mesh_[index].Draw(cList);
	}

}

void Model::SetWVP()
{
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	Matrix4x4 viewMatrix = DXCom::GetInstance()->GetView();

	Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, DXCom::GetInstance()->GetAspect(), 0.1f, 100.0f);
	Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));

	wvpDate_->World = worldMatrix;
	wvpDate_->WVP = worldViewProjectionMatrix;
}

void Model::AddMaterial(const Material& material) {
	material_.push_back(material);
}

void Model::AddMesh(const Mesh& mesh) {
	wvpResource_ = DXCom::GetInstance()->CreateBufferResource(DXCom::GetInstance()->GetDevice(), sizeof(TransformationMatrix));
	wvpDate_ = nullptr;
	wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpDate_));
	wvpDate_->WVP = MakeIdentity4x4();
	wvpDate_->World = MakeIdentity4x4();

	mesh_.push_back(mesh);
}
