#include "Material.h"
#include "Engine/DX/DXCom.h"



Material::Material() {
	dxcommon_ = TextureManager::GetInstance()->ShareDXCom();
}
Material::~Material() {
	dxcommon_ = nullptr;

	materialResource_.Reset();
	materialEnvironmentResource_.Reset();

}

void Material::Finalize() {
	dxcommon_ = nullptr;

	materialResource_.Reset();
	materialEnvironmentResource_.Reset();
}


void Material::CreateMaterial() {
	materialResource_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(MaterialDate));
	materialDate_ = nullptr;
	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialDate_));
	//色変えるやつ（Resource）
	materialDate_->color = { 1.0f,1.0f,1.0f,1.0f };
	materialDate_->enableLighting = static_cast<int32_t>(LightMode::kSpotLightON);
	materialDate_->uvTransform = MakeIdentity4x4();
	materialDate_->shininess = 50.0f;
	materialDate_->AlphaRef = 0.5f;

	if (textureNamePath_.textureFilePath.empty()) {
		texture_ = TextureManager::GetInstance()->LoadTexture("uvChecker.png");
	} else {
		texture_ = TextureManager::GetInstance()->LoadTexture(textureNamePath_.textureFilePath);
	}
}

void Material::CreateEnvironmentMaterial() {
	isEnvironment_ = true;

	materialEnvironmentResource_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(MaterialEnvironment));
	materialEnvironment_ = nullptr;
	materialEnvironmentResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialEnvironment_));
	//色変えるやつ（Resource）
	materialEnvironment_->color = materialDate_->color;
	materialEnvironment_->enableLighting = static_cast<int32_t>(LightMode::kSpotLightON);
	materialEnvironment_->uvTransform = MakeIdentity4x4();
	materialEnvironment_->shininess = 50.0f;
	materialEnvironment_->environmentCoefficient = 0.0f;
}


Texture* Material::GetTexture() {
	return texture_;
}


ID3D12Resource* Material::GetMaterialResource() {
	if (isEnvironment_) {
		return materialEnvironmentResource_.Get();
	} else {
		return materialResource_.Get();
	}
}


void Material::SetTextureNamePath(const std::string& pathName) {
	textureNamePath_.textureFilePath = pathName;
}

void Material::SetUVScale(const Vector2& scale, const Vector2& uvTrans) {
	Matrix4x4 uvScaleMatrix = MakeScale4x4(Vector3(scale.x, scale.y, 1.0f));
	Matrix4x4 uvTransMatrix = MakeTranslateMatrix(Vector3(uvTrans.x, uvTrans.y, 0.0f));
	materialDate_->uvTransform = MakeIdentity4x4();
	materialDate_->uvTransform = Multiply(uvTransMatrix, uvScaleMatrix);
}

void Material::SetTexture(const std::string& name) {
	texture_ = TextureManager::GetInstance()->GetTexture(name);
}

void Material::SetLightEnable(LightMode mode) {
	materialDate_->enableLighting = static_cast<int32_t>(mode);
}

void Material::SetEnvironment(float env) {
	if (isEnvironment_) {
		materialEnvironment_->environmentCoefficient = env;
	} else {
		materialDate_->shininess = env;
	}
}

Matrix4x4 Material::MakeScale4x4(const Vector3& scale) {
	Matrix4x4 scaleMatrix = MakeIdentity4x4();

	scaleMatrix.m[0][0] = scale.x; // X軸のスケール
	scaleMatrix.m[1][1] = scale.y; // Y軸のスケール
	scaleMatrix.m[2][2] = scale.z; // Z軸のスケール

	return scaleMatrix;
}