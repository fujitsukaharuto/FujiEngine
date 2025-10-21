#pragma once
#include "Math/Matrix/MatrixCalculation.h"
#include "TextureManager.h"

class DXCom;

/// <summary>
/// TextureのVertexDeta
/// </summary>
struct VertexDate {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};

/// <summary>
/// Textureのファイルパス
/// </summary>
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

/// <summary>
/// マテリアルクラス
/// </summary>
class Material {
public:
	/// <summary>
	/// マテリアルのデータ
	/// </summary>
	struct MaterialDate {
		Vector4 color;
		int32_t enableLighting;
		float padding[3];
		Matrix4x4 uvTransform;
		float shininess;
		float AlphaRef = 0.5f;
	};
	
	/// <summary>
	/// 環境マップのデータ
	/// </summary>
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

	/// <summary>マテリアルの生成</summary>
	void CreateMaterial();

	/// <summary>環境マップの生成</summary>
	void CreateEnvironmentMaterial();

	//========================================================================*/
	//* Getter
	Texture* GetTexture();
	ID3D12Resource* GetMaterialResource();
	std::string GetPathName() const { return textureNamePath_.textureFilePath; }
	Vector4 GetColor() { return materialDate_->color; }

	//========================================================================*/
	//* Setter
	/// <summary>パスの設定</summary>
	void SetTextureNamePath(const std::string& pathName);
	/// <summary>色の設定</summary>
	void SetColor(const Vector4& color) { materialDate_->color = color; }
	/// <summary>UVスケールの設定</summary>
	void SetUVScale(const Vector2& scale, const Vector2& uvTrans);
	/// <summary>アルファ閾値の設定</summary>
	void SetAlphaRef(float ref) { materialDate_->AlphaRef = ref; }
	/// <summary>テクスチャの設定</summary>
	void SetTexture(const std::string& name, bool overWrite = false);
	/// <summary>ライトモードの設定</summary>
	void SetLightEnable(LightMode mode);
	/// <summary>環境マップの設定</summary>
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