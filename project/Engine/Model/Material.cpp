#include "Material.h"
#include "DXCom.h"



Material::Material() {}
Material::~Material() {}


void Material::CreateMaterial() {
	materialResource_ = DXCom::GetInstance()->CreateBufferResource(DXCom::GetInstance()->GetDevice(), sizeof(MaterialDate));
	materialDate_ = nullptr;
	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialDate_));
	//色変えるやつ（Resource）
	materialDate_->color = { 1.0f,1.0f,1.0f,1.0f };
	materialDate_->enableLighting = LightMode::kLightHalfLambert;
	materialDate_->uvTransform = MakeIdentity4x4();
	materialDate_->shininess = 50.0f;

	if (textureNamePath_.textureFilePath.empty()) {
		texture_ = TextureManager::GetInstance()->LoadTexture("uvChecker.png");
	}
	else {
		texture_ = TextureManager::GetInstance()->LoadTexture(textureNamePath_.textureFilePath);
	}

}


Texture* Material::GetTexture() {
	return texture_;
}


ID3D12Resource* Material::GetMaterialResource() {
	return materialResource_.Get();
}


void Material::SetTextureNamePath(const std::string& pathName) {
	textureNamePath_.textureFilePath = pathName;
}

void Material::SetUVScale(const Vector2& scale) {

	Matrix4x4 uvScaleMatrix = MakeScale4x4(Vector3(scale.x, scale.y, 1.0f));
	materialDate_->uvTransform = MakeIdentity4x4();
	materialDate_->uvTransform = Multiply(uvScaleMatrix, materialDate_->uvTransform);

}

void Material::SetTexture(const std::string& name) {

	texture_ = TextureManager::GetInstance()->GetTexture(name);

}

void Material::SetLightEnable(LightMode mode) {
	materialDate_->enableLighting = mode;
}

Matrix4x4 Material::MakeScale4x4(const Vector3& scale) {
	Matrix4x4 scaleMatrix = MakeIdentity4x4();

	scaleMatrix.m[0][0] = scale.x; // X軸のスケール
	scaleMatrix.m[1][1] = scale.y; // Y軸のスケール
	scaleMatrix.m[2][2] = scale.z; // Z軸のスケール

	return scaleMatrix;
}
