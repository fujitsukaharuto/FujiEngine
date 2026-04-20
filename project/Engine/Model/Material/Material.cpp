#include "Material.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/DX12Helper.h"

using namespace Graphics;
using namespace Math;


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
		materialDateGPU_[i] = nullptr;
		materialResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&materialDateGPU_[i]));
	}

	//色変えるやつ（Resource）
	materialDate_.color = Colors::White;
	materialDate_.enableLighting = static_cast<int32_t>(LightMode::kPointLightON);
	materialDate_.uvTransform = MakeIdentity4x4();
	materialDate_.environmentCoefficient = 0.0f;
	materialDate_.useNormalMap = 0;
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
	return materialDate_.color;
}

void Material::SetTextureNamePath(const std::string& pathName) {
	textureNamePath_.textureFilePath = pathName;
}

void Material::SetColor(const Math::Vector4& color) {
	materialDate_.color = color;
}

void Material::SetUVScale(const Vector2& scale, const Vector2& uvTrans) {
	Matrix4x4 uvScaleMatrix = MakeScale4x4(Vector3(scale.x, scale.y, 1.0f));
	Matrix4x4 uvTransMatrix = MakeTranslateMatrix(Vector3(uvTrans.x, uvTrans.y, 0.0f));
	scale_ = scale;
	uvTrans_ = uvTrans;

	materialDate_.uvTransform = MakeIdentity4x4();
	materialDate_.uvTransform = Multiply(uvTransMatrix, uvScaleMatrix);
}

void Material::SetUVTrans(const Vector2& uvTrans) {
	Matrix4x4 uvScaleMatrix = MakeScale4x4(Vector3(scale_.x, scale_.y, 1.0f));
	Matrix4x4 uvTransMatrix = MakeTranslateMatrix(Vector3(uvTrans.x, uvTrans.y, 0.0f));
	uvTrans_ = uvTrans;
	materialDate_.uvTransform = MakeIdentity4x4();
	materialDate_.uvTransform = Multiply(uvTransMatrix, uvScaleMatrix);
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
	materialDate_.useNormalMap = 1;
}

void Material::SetLightEnable(LightMode mode) {
	materialDate_.enableLighting = static_cast<int32_t>(mode);
}

void Graphics::Material::SetShininess(float shininess) {
	materialDate_.shininess = shininess;
}

void Material::SetEnvironment(float env) {
	materialDate_.environmentCoefficient = env;
}

void Material::CopyData(uint32_t frameIndex) {
	materialDateGPU_[frameIndex]->color = materialDate_.color;
	materialDateGPU_[frameIndex]->enableLighting = materialDate_.enableLighting;
	materialDateGPU_[frameIndex]->uvTransform = materialDate_.uvTransform;
	materialDateGPU_[frameIndex]->shininess = materialDate_.shininess;
	materialDateGPU_[frameIndex]->AlphaRef = materialDate_.AlphaRef;
	materialDateGPU_[frameIndex]->environmentCoefficient = materialDate_.environmentCoefficient;
	materialDateGPU_[frameIndex]->useNormalMap = materialDate_.useNormalMap;
}

Matrix4x4 Material::MakeScale4x4(const Vector3& scale) {
	Matrix4x4 scaleMatrix = MakeIdentity4x4();

	scaleMatrix.m[0][0] = scale.x; // X軸のスケール
	scaleMatrix.m[1][1] = scale.y; // Y軸のスケール
	scaleMatrix.m[2][2] = scale.z; // Z軸のスケール

	return scaleMatrix;
}