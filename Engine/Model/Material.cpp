#include "Material.h"
#include "DXCom.h"



Material::Material() {}
Material::~Material() {}


void Material::CreateMaterial()
{
	materialResource_ = DXCom::GetInstance()->CreateBufferResource(DXCom::GetInstance()->GetDevice(), sizeof(MaterialDate));
	materialDate_ = nullptr;
	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialDate_));
	//色変えるやつ（Resource）
	materialDate_->color = { 1.0f,1.0f,1.0f,1.0f };
	materialDate_->enableLighting = LightMode::kLightHalfLambert;
	materialDate_->uvTransform = MakeIdentity4x4();
	materialDate_->shininess = 50.0f;

	if (textureNamePath_.textureFilePath.empty())
	{
		texture_ = TextureManager::GetInstance()->LoadTexture("white2x2.png");
	}
	else
	{
		texture_ = TextureManager::GetInstance()->LoadTexture(textureNamePath_.textureFilePath);
	}

}


Texture* Material::GetTexture()
{
	return texture_;
}


ID3D12Resource* Material::GetMaterialResource()
{
	return materialResource_.Get();
}


void Material::SetTextureNamePath(const std::string& pathName)
{
	textureNamePath_.textureFilePath = pathName;
}

void Material::SetTexture(const std::string& name) {

	texture_ = TextureManager::GetInstance()->GetTexture(name);

}
