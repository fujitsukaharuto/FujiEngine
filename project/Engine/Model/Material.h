#pragma once
#include "MatrixCalculation.h"
#include "TextureManager.h"

enum LightMode {
	kLightNone,
	kLightHalfLambert,
	kLightLambert,
	kPhongReflect,
	kBlinnPhongReflection,
	kPointLightON,
	kSpotLightON,
};


class Material
{
public:
	struct MaterialDate
	{
		Vector4 color;
		int32_t enableLighting;
		float padding[3];
		Matrix4x4 uvTransform;
		float shininess;
	};

public:
	Material();
	~Material();


	void CreateMaterial();

	Texture* GetTexture();

	ID3D12Resource* GetMaterialResource();

	std::string GetPathName() const { return textureNamePath_.textureFilePath; }

	void SetTextureNamePath(const std::string& pathName);

	void SetColor(const Vector4& color) { materialDate_->color = color; }

	void SetUVScale(const Vector2& scale, const Vector2& uvTrans);

	void SetTexture(const std::string& name);

	void SetLightEnable(LightMode mode);

private:

	Matrix4x4 MakeScale4x4(const Vector3& scale);

	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_ = nullptr;
	MaterialDate* materialDate_ = nullptr;
	Texture* texture_ = nullptr;
	MaterialDataPath textureNamePath_;
};
