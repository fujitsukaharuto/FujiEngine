#include "Model.h"
#include "DXCom.h"

Model::Model() {}

Model::~Model() {}

Model* Model::CreateSphere()
{

	Model* model = new Model();

	Mesh mesh{};
	Material material{};


	const float pi = 3.1415926535f;
	const uint32_t kSubdivision = 16;
	const float kLonEvery = (pi * 2.0f) / static_cast<float>(kSubdivision);
	const float kLatEvery = (pi) / static_cast<float>(kSubdivision);

	for (uint32_t latIndex = 0; latIndex < kSubdivision; latIndex++)
	{
		float lat = -pi / 2.0f + kLatEvery * latIndex;
		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; lonIndex++)
		{
			float lon = lonIndex * kLonEvery;

			float u = float(lonIndex) / float(kSubdivision);
			float v = 1.0f - float(latIndex) / float(kSubdivision);


			mesh.AddVertex({ {(cosf(lat) * cosf(lon)), (sinf(lat)), (cosf(lat) * sinf(lon)), 1.0f},
				{u, v},
				{(cosf(lat) * cosf(lon)), (sinf(lat)), (cosf(lat) * sinf(lon))} });


			mesh.AddVertex({ {(cosf(lat + kLatEvery) * cosf(lon)), (sinf(lat + kLatEvery)), (cosf(lat + kLatEvery) * sinf(lon)), 1.0f},
				{u,v - (float(1.0f) / float(kSubdivision))},
				{(cosf(lat + kLatEvery) * cosf(lon)), (sinf(lat + kLatEvery)), (cosf(lat + kLatEvery) * sinf(lon))} });


			mesh.AddVertex({ {(cosf(lat) * cosf(lon + kLonEvery)), (sinf(lat)), (cosf(lat) * sinf(lon + kLonEvery)), 1.0f},
				{u + (float(1.0f) / float(kSubdivision)),v},
				{(cosf(lat) * cosf(lon + kLonEvery)), (sinf(lat)), (cosf(lat) * sinf(lon + kLonEvery))} });


			mesh.AddVertex({ {(cosf(lat + kLatEvery) * cosf(lon)), (sinf(lat + kLatEvery)), (cosf(lat + kLatEvery) * sinf(lon)), 1.0f},
				{u,v - (float(1.0f) / float(kSubdivision))},
				{(cosf(lat + kLatEvery) * cosf(lon)), (sinf(lat + kLatEvery)), (cosf(lat + kLatEvery) * sinf(lon))} });


			mesh.AddVertex({ {(cosf(lat + kLatEvery) * cosf(lon + kLonEvery)), (sinf(lat + kLatEvery)), (cosf(lat + kLatEvery) * sinf(lon + kLonEvery)), 1.0f},
				{u + (float(1.0f) / float(kSubdivision)),v - (float(1.0f) / float(kSubdivision))},
				{(cosf(lat + kLatEvery) * cosf(lon + kLonEvery)), (sinf(lat + kLatEvery)), (cosf(lat + kLatEvery) * sinf(lon + kLonEvery))} });


			mesh.AddVertex({ {(cosf(lat) * cosf(lon + kLonEvery)), (sinf(lat)), (cosf(lat) * sinf(lon + kLonEvery)), 1.0f},
				{u + (float(1.0f) / float(kSubdivision)),v},
				{(cosf(lat) * cosf(lon + kLonEvery)), (sinf(lat)), (cosf(lat) * sinf(lon + kLonEvery))} });
		}
	}


	mesh.CreateMesh();
	material.CreateMaterial();

	
	model->wvpResource_ = DXCom::GetInstance()->CreateBufferResource(DXCom::GetInstance()->GetDevice(), sizeof(TransformationMatrix));
	model->wvpDate_ = nullptr;
	model->wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&model->wvpDate_));
	model->wvpDate_->WVP = MakeIdentity4x4();
	model->wvpDate_->World = MakeIdentity4x4();

	model->mesh_.push_back(mesh);
	model->material_.push_back(material);

	return model;
}

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
