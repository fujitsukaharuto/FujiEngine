#include "Material.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"

using namespace Graphics;
using namespace Math;
using namespace DXC;


Material::Material() {
	dxcommon_ = TextureManager::GetInstance()->ShareDXCom();
}
Material::~Material() {
	dxcommon_ = nullptr;

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		materialResource_[i].Reset();
	}
}

void Material::Finalize() {
	dxcommon_ = nullptr;

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		materialResource_[i].Reset();
	}
}


void Material::CreateMaterial() {
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		materialResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(MaterialData));
		materialDataGPU_[i] = nullptr;
		materialResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&materialDataGPU_[i]));
	}

	//色変えるやつ（Resource）
	materialData_.color = Colors::White;
	materialData_.enableLighting = static_cast<int32_t>(LightMode::kPointLightON);
	materialData_.uvTransform = MakeIdentity4x4();
	materialData_.environmentCoefficient = 0.0f;
	materialData_.useNormalMap = 0;
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		CopyData(i);
	}

	scale_ = { 1.0f,1.0f };
	uvTrans_ = { 0.0f,0.0f };

	if (textureNamePath_.textureFilePath.empty()) {
		texture_ = TextureManager::GetInstance()->LoadTexture("uvChecker.png");
	} else {
		texture_ = TextureManager::GetInstance()->LoadTexture(textureNamePath_.textureFilePath);
	}
	
	// Default normal map
	normalMap_ = TextureManager::GetInstance()->GetTexture("defaultNormal.png");
}

Texture* Material::GetTexture() {
	return texture_;
}

ID3D12Resource* Material::GetMaterialResource() {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	if (!materialResource_[frameIndex]) {
		return nullptr;
	}
	CopyData(frameIndex);
	return materialResource_[frameIndex].Get();
}

Vector4 Material::GetColor() {
	return materialData_.color;
}

void Material::SetTextureNamePath(const std::string& pathName) {
	textureNamePath_.textureFilePath = pathName;
}

void Material::SetColor(const Math::Vector4& color) {
	materialData_.color = color;
}

void Material::SetUVScale(const Vector2& scale, const Vector2& uvTrans) {
	Matrix4x4 uvScaleMatrix = MakeScale4x4(Vector3(scale.x, scale.y, 1.0f));
	Matrix4x4 uvTransMatrix = MakeTranslateMatrix(Vector3(uvTrans.x, uvTrans.y, 0.0f));
	scale_ = scale;
	uvTrans_ = uvTrans;

	materialData_.uvTransform = MakeIdentity4x4();
	materialData_.uvTransform = Multiply(uvTransMatrix, uvScaleMatrix);
}

void Material::SetUVTrans(const Vector2& uvTrans) {
	Matrix4x4 uvScaleMatrix = MakeScale4x4(Vector3(scale_.x, scale_.y, 1.0f));
	Matrix4x4 uvTransMatrix = MakeTranslateMatrix(Vector3(uvTrans.x, uvTrans.y, 0.0f));
	uvTrans_ = uvTrans;
	materialData_.uvTransform = MakeIdentity4x4();
	materialData_.uvTransform = Multiply(uvTransMatrix, uvScaleMatrix);
}

void Material::SetTexture(const std::string& name, bool overWrite) {
	if (overWrite) {
		TextureManager::GetInstance()->Load(name, overWrite);
	}
	texture_ = TextureManager::GetInstance()->GetTexture(name);
}

void Material::SetNormalMap(const std::string& name, bool overWrite) {
	if (overWrite) {
		TextureManager::GetInstance()->Load(name, overWrite);
	}
	normalMap_ = TextureManager::GetInstance()->GetTexture(name);
	materialData_.useNormalMap = 1;
}

void Material::SetLightEnable(LightMode mode) {
	materialData_.enableLighting = static_cast<int32_t>(mode);
}

void Graphics::Material::SetShininess(float shininess) {
	materialData_.shininess = shininess;
}

void Material::SetEnvironment(float env) {
	materialData_.environmentCoefficient = env;
}

void Material::CopyData(uint32_t frameIndex) {
	materialDataGPU_[frameIndex]->color = materialData_.color;
	materialDataGPU_[frameIndex]->enableLighting = materialData_.enableLighting;
	materialDataGPU_[frameIndex]->uvTransform = materialData_.uvTransform;
	materialDataGPU_[frameIndex]->shininess = materialData_.shininess;
	materialDataGPU_[frameIndex]->AlphaRef = materialData_.AlphaRef;
	materialDataGPU_[frameIndex]->environmentCoefficient = materialData_.environmentCoefficient;
	materialDataGPU_[frameIndex]->useNormalMap = materialData_.useNormalMap;
	materialDataGPU_[frameIndex]->textureIndex = texture_ ? texture_->srvIndex : 0;
	materialDataGPU_[frameIndex]->normalMapIndex = normalMap_ ? normalMap_->srvIndex : 0;
}

Matrix4x4 Material::MakeScale4x4(const Vector3& scale) {
	Matrix4x4 scaleMatrix = MakeIdentity4x4();

	scaleMatrix.m[0][0] = scale.x; // X軸のスケール
	scaleMatrix.m[1][1] = scale.y; // Y軸のスケール
	scaleMatrix.m[2][2] = scale.z; // Z軸のスケール

	return scaleMatrix;
}