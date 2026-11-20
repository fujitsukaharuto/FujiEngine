#include "Material.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include <algorithm>
#include "Engine/Graphics/Texture/TextureManager.h"

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

uint32_t Material::GetTextureSrvIndex() const {
	return texture_ ? texture_->srvIndex : 0;
}

uint32_t Material::GetEmissiveTextureSrvIndex() const {
	return emissiveMap_ ? emissiveMap_->srvIndex : GetTextureSrvIndex();
}

bool Material::IsCutout() const {
	return texture_ && texture_->hasTransparency && materialData_.AlphaRef > 0.0f;
}

ID3D12Resource* Material::UploadAndGetResource() {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	if (!materialResource_[frameIndex]) {
		return nullptr;
	}
	CopyData(frameIndex);
	return materialResource_[frameIndex].Get();
}

Vector4 Material::GetColor() const {
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
	normalMap_ = TextureManager::GetInstance()->GetLinearTexture(name, overWrite);
	materialData_.useNormalMap = 1;
}

void Material::SetMetallicRoughnessMap(const std::string& name) {
	// 数値なので sRGB として読まない
	metallicRoughnessMap_ = TextureManager::GetInstance()->GetLinearTexture(name, false);
	materialData_.useMetallicRoughnessMap = metallicRoughnessMap_ ? 1 : 0;
}

void Material::SetEmissiveMap(const std::string& name) {
	// 色なので sRGB として読む
	emissiveMap_ = name.empty() ? nullptr : TextureManager::GetInstance()->GetTexture(name);
	materialData_.useEmissiveMap = emissiveMap_ ? 1 : 0;
}

void Material::SetOcclusionMap(const std::string& name) {
	occlusionMap_ = TextureManager::GetInstance()->GetLinearTexture(name, false);
	materialData_.useOcclusionMap = occlusionMap_ ? 1 : 0;
}

void Material::SetHeightMap(const std::string& name, float scale) {
	// 高さは色ではないので sRGB にしない
	heightMap_ = name.empty() ? nullptr : TextureManager::GetInstance()->GetLinearTexture(name, false);
	materialData_.useHeightMap = heightMap_ ? 1 : 0;
	materialData_.heightScale = scale;
}

void Material::SetLightEnable(LightMode mode) {
	materialData_.enableLighting = static_cast<int32_t>(mode);
}

void Graphics::Material::SetRoughness(float roughness) {
	materialData_.roughness = std::clamp(roughness, 0.0f, 1.0f);
}

void Graphics::Material::SetMetallic(float metallic) {
	materialData_.metallic = std::clamp(metallic, 0.0f, 1.0f);
}

void Material::SetEnvironment(float env) {
	materialData_.environmentCoefficient = env;
}

void Material::CopyData(uint32_t frameIndex) {
	materialDataGPU_[frameIndex]->color = materialData_.color;
	materialDataGPU_[frameIndex]->enableLighting = materialData_.enableLighting;
	materialDataGPU_[frameIndex]->uvTransform = materialData_.uvTransform;
	materialDataGPU_[frameIndex]->roughness = materialData_.roughness;
	materialDataGPU_[frameIndex]->AlphaRef = materialData_.AlphaRef;
	materialDataGPU_[frameIndex]->environmentCoefficient = materialData_.environmentCoefficient;
	materialDataGPU_[frameIndex]->useNormalMap = materialData_.useNormalMap;
	materialDataGPU_[frameIndex]->textureIndex = texture_ ? texture_->srvIndex : 0;
	materialDataGPU_[frameIndex]->normalMapIndex = normalMap_ ? normalMap_->srvIndex : 0;
	materialDataGPU_[frameIndex]->metallic = materialData_.metallic;
	materialDataGPU_[frameIndex]->emissive = materialData_.emissive;
	materialDataGPU_[frameIndex]->rimPower = materialData_.rimPower;
	materialDataGPU_[frameIndex]->rimColor = materialData_.rimColor;
	materialDataGPU_[frameIndex]->metallicRoughnessIndex = metallicRoughnessMap_ ? static_cast<int32_t>(metallicRoughnessMap_->srvIndex) : 0;
	materialDataGPU_[frameIndex]->useMetallicRoughnessMap = materialData_.useMetallicRoughnessMap;
	materialDataGPU_[frameIndex]->isTwoSided = materialData_.isTwoSided;
	materialDataGPU_[frameIndex]->weathering = materialData_.weathering;
	materialDataGPU_[frameIndex]->flowSpeed = materialData_.flowSpeed;
	materialDataGPU_[frameIndex]->emissiveMapIndex = emissiveMap_ ? static_cast<int32_t>(emissiveMap_->srvIndex) : 0;
	materialDataGPU_[frameIndex]->useEmissiveMap = materialData_.useEmissiveMap;
	materialDataGPU_[frameIndex]->occlusionMapIndex = occlusionMap_ ? static_cast<int32_t>(occlusionMap_->srvIndex) : 0;
	materialDataGPU_[frameIndex]->useOcclusionMap = materialData_.useOcclusionMap;
	materialDataGPU_[frameIndex]->clearCoat = materialData_.clearCoat;
	materialDataGPU_[frameIndex]->clearCoatRoughness = materialData_.clearCoatRoughness;
	materialDataGPU_[frameIndex]->anisotropy = materialData_.anisotropy;
	materialDataGPU_[frameIndex]->anisotropyRotation = materialData_.anisotropyRotation;
	materialDataGPU_[frameIndex]->sheenColor = materialData_.sheenColor;
	materialDataGPU_[frameIndex]->sheenRoughness = materialData_.sheenRoughness;
	materialDataGPU_[frameIndex]->heightMapIndex = heightMap_ ? static_cast<int32_t>(heightMap_->srvIndex) : 0;
	materialDataGPU_[frameIndex]->useHeightMap = materialData_.useHeightMap;
	materialDataGPU_[frameIndex]->heightScale = materialData_.heightScale;
	materialDataGPU_[frameIndex]->meshIndex = materialData_.meshIndex;
}

Matrix4x4 Material::MakeScale4x4(const Vector3& scale) {
	Matrix4x4 scaleMatrix = MakeIdentity4x4();

	scaleMatrix.m[0][0] = scale.x; // X軸のスケール
	scaleMatrix.m[1][1] = scale.y; // Y軸のスケール
	scaleMatrix.m[2][2] = scale.z; // Z軸のスケール

	return scaleMatrix;
}