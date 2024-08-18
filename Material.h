#pragma once
#include "MatrixCalculation.h"
#include "TextureManager.h"

class Material
{
public:
	struct MaterialDate
	{
		Vector4 color;
		int32_t enableLighting;
		float padding[3];
		Matrix4x4 uvTransform;
	};

public:
	Material();
	~Material();


	void CreateMaterial();

	Texture* GetTexture();

	ID3D12Resource* GetMaterialResource();

	ID3D12Resource* GetDirectionLight();

private:

	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_ = nullptr;
	MaterialDate* materialDate_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_ = nullptr;
	DirectionalLight* directionalLightData_ = nullptr;
	Texture* texture_ = nullptr;
};
