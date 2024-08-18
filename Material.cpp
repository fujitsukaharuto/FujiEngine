#include "Material.h"
#include "DXCom.h"

//Material::Material() {}
//
//Material::~Material() {}

Material::Material()
{
}

Material::~Material()
{
}

void Material::CreateMaterial()
{
	materialResource_ = DXCom::GetInstance()->CreateBufferResource(DXCom::GetInstance()->GetDevice(), sizeof(MaterialDate));
	materialDate_ = nullptr;
	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialDate_));
	//色変えるやつ（Resource）
	materialDate_->color = { 1.0f,1.0f,1.0f,1.0f };
	materialDate_->enableLighting = LightMode::kLightHalfLambert;
	materialDate_->uvTransform = MakeIdentity4x4();

	directionalLightResource_ = DXCom::GetInstance()->CreateBufferResource(DXCom::GetInstance()->GetDevice(), sizeof(DirectionalLight));
	directionalLightData_ = nullptr;
	directionalLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData_));
	directionalLightData_->color = { 1.0f,1.0f,1.0f,1.0f };
	directionalLightData_->direction = { 1.0f,0.0f,0.0f };
	directionalLightData_->intensity = 1.0f;

	texture_ = TextureManager::GetInstance()->LoadTexture("uvChecker.png");

}

Texture* Material::GetTexture()
{
	return texture_;
}

ID3D12Resource* Material::GetMaterialResource()
{
	return materialResource_.Get();
}

ID3D12Resource* Material::GetDirectionLight()
{
	return directionalLightResource_.Get();
}
