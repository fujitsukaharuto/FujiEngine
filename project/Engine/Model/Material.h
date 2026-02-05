#pragma once
#include "Math/Matrix/MatrixCalculation.h"
#include "Engine/Math/Vector/Vector4.h"
#include "TextureManager.h"
#include "Engine/DX/FrameCount.h"

class DXCom;

/// <summary>
/// TextureのVertexData
/// </summary>
struct VertexDate {
	Math::Vector4 position;
	Math::Vector2 texcoord;
	Math::Vector3 normal;
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
		struct MaterialDate {
			Math::Vector4 color;
			int32_t enableLighting;
			float padding[3];
			Math::Matrix4x4 uvTransform;
			float shininess = 50.0f;
			float AlphaRef = 0.5f;
		};

		/// <summary>
		/// 環境マップのデータ
		/// </summary>
		struct MaterialEnvironment {
			Math::Vector4 color;
			int32_t enableLighting;
			float padding[3];
			Math::Matrix4x4 uvTransform;
			float shininess = 50.0f;
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
		Math::Vector4 GetColor();
		Math::Vector2 GetUVTrans() { return uvTrans_; }

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
		/// <summary>アルファ閾値の設定</summary>
		void SetAlphaRef(float ref) { materialDate_.AlphaRef = ref; }
		/// <summary>テクスチャの設定</summary>
		void SetTexture(const std::string& name, bool overWrite = false);
		/// <summary>ライトモードの設定</summary>
		void SetLightEnable(LightMode mode);
		/// <summary>環境マップの設定</summary>
		void SetEnvironment(float env);

	private:

		void CopyData(uint32_t frameIndex = 0);
		void CopyDataEnvironment(uint32_t frameIndex = 0);

		DXCom* dxcommon_;

		Math::Matrix4x4 MakeScale4x4(const Math::Vector3& scale);

		Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_[DXC::kFrameCount_];
		MaterialDate* materialDateGPU_[DXC::kFrameCount_];
		MaterialDate materialDate_;

		Microsoft::WRL::ComPtr<ID3D12Resource> materialEnvironmentResource_[DXC::kFrameCount_];
		MaterialEnvironment* materialEnvironmentGPU_[DXC::kFrameCount_];
		MaterialEnvironment materialEnvironment_;
		bool isEnvironment_ = false;

		Math::Vector2 scale_;
		Math::Vector2 uvTrans_;

		Texture* texture_ = nullptr;
		MaterialDataPath textureNamePath_;
	};
}