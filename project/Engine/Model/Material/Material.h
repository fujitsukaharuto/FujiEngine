#pragma once
#include "Math/Matrix/MatrixCalculation.h"
#include "Engine/Math/Vector/Vector4.h"
#include "TextureManager.h"
#include "Engine/DX/FrameCount.h"

class DXCom;

/// <summary>
/// Textureのファイルパス
/// </summary>
struct MaterialDataPath {
	std::string textureFilePath;
	std::string normalFilePath;
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

namespace Colors {
	constexpr Math::Vector4 Black = { 0.0f, 0.0f, 0.0f, 1.0f };
	constexpr Math::Vector4 White = { 1.0f, 1.0f, 1.0f, 1.0f };
	constexpr Math::Vector4 Red = { 1.0f, 0.0f, 0.0f, 1.0f };
	constexpr Math::Vector4 Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
	constexpr Math::Vector4 Transparent = { 0.0f, 0.0f, 0.0f, 0.0f };
}

namespace Graphics {
	/// <summary>
	/// マテリアルクラス
	/// </summary>
	class Material {
	public:
		/// <summary>
		/// マテリアルのデータ
		/// </summary>
		struct MaterialData {
			Math::Vector4 color;
			Math::Matrix4x4 uvTransform;
			int32_t enableLighting;
			float shininess = 50.0f;
			float AlphaRef = 0.5f;
			float environmentCoefficient = 1.0f;
			int32_t useNormalMap = 0;
			int32_t textureIndex = 0;
			int32_t normalMapIndex = 0;
		};

	public:
		Material();
		~Material();
		void Finalize();

		/// <summary>マテリアルの生成</summary>
		void CreateMaterial();

		//========================================================================*/
		//* Getter
		Texture* GetTexture();
		Texture* GetNormalMap() { return normalMap_; }
		ID3D12Resource* GetMaterialResource();
		std::string GetPathName() const { return textureNamePath_.textureFilePath; }
		Math::Vector4 GetColor();
		Math::Vector2 GetUVScale() { return scale_; }
		Math::Vector2 GetUVTrans() { return uvTrans_; }
		int32_t GetUseNormalMap() { return materialData_.useNormalMap; }

		//========================================================================*/
		//* Setter
		/// <summary>パスの設定</summary>
		void SetTextureNamePath(const std::string& pathName);
		/// <summary>色の設定</summary>
		void SetColor(const Math::Vector4& color);
		/// <summary>UVスケールの設定</summary>
		void SetUVScale(const Math::Vector2& scale, const Math::Vector2& uvTrans);
		/// <summary>UVトランスの設定</summary>
		void SetUVTrans(const Math::Vector2& uvTrans);
		/// <summary> 法線マップを使用するか</summary>
		void SetUseNormalMap(int32_t is) { materialData_.useNormalMap = is; }
		/// <summary>アルファ閾値の設定</summary>
		void SetAlphaRef(float ref) { materialData_.AlphaRef = ref; }
		/// <summary>テクスチャの設定</summary>
		void SetTexture(const std::string& name, bool overWrite = false);
		/// <summary>法線マップの設定</summary>
		void SetNormalMap(const std::string& name, bool overWrite = false);
		/// <summary>ライトモードの設定</summary>
		void SetLightEnable(LightMode mode);
		/// <summary>反射の設定</summary>
		void SetShininess(float shininess);
		/// <summary>環境マップの設定</summary>
		void SetEnvironment(float env);

	private:

		void CopyData(uint32_t frameIndex = 0);

		DXCom* dxcommon_;

		Math::Matrix4x4 MakeScale4x4(const Math::Vector3& scale);

		Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_[DXC::kFrameCount_];
		MaterialData* materialDataGPU_[DXC::kFrameCount_];
		MaterialData materialData_;

		Math::Vector2 scale_;
		Math::Vector2 uvTrans_;

		Texture* texture_ = nullptr;
		Texture* normalMap_ = nullptr;
		MaterialDataPath textureNamePath_;
	};
}