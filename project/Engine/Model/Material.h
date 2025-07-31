#pragma once
#include "Math/Matrix/MatrixCalculation.h"
#include "TextureManager.h"

class DXCom;

struct VertexDate {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};

struct MaterialDataPath {
	std::string textureFilePath;
};

enum class LightMode {
	kLightNone,
	kLightHalfLambert,
	kLightLambert,
	kPhongReflect,
	kBlinnPhongReflection,
	kPointLightON,
	kSpotLightON,
};

class Material {
public:
	struct MaterialDate {
		Vector4 color;
		int32_t enableLighting;
		float padding[3];
		Matrix4x4 uvTransform;
		float shininess;
		float AlphaRef = 0.5f;
	};

	struct MaterialEnvironment {
		Vector4 color;
		int32_t enableLighting;
		float padding[3];
		Matrix4x4 uvTransform;
		float shininess;
		float environmentCoefficient = 1.0f;
	};

public:
	Material();
	~Material();
	void Finalize();

	void CreateMaterial();

	void CreateEnvironmentMaterial();

	Texture* GetTexture();

	ID3D12Resource* GetMaterialResource();

	std::string GetPathName() const { return textureNamePath_.textureFilePath; }

	Vector4 GetColor() { return materialDate_->color; }

	void SetTextureNamePath(const std::string& pathName);

	void SetColor(const Vector4& color) { materialDate_->color = color; }

	void SetUVScale(const Vector2& scale, const Vector2& uvTrans);

	void SetAlphaRef(float ref) { materialDate_->AlphaRef = ref; }

	void SetTexture(const std::string& name);

	void SetLightEnable(LightMode mode);

	void SetEnvironment(float env);

private:

	DXCom* dxcommon_;

	Matrix4x4 MakeScale4x4(const Vector3& scale);

	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_ = nullptr;
	MaterialDate* materialDate_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> materialEnvironmentResource_ = nullptr;
	MaterialEnvironment* materialEnvironment_ = nullptr;
	bool isEnvironment_ = false;

	Texture* texture_ = nullptr;
	MaterialDataPath textureNamePath_;
};