#pragma once
#include <string>
#include <d3d12.h>
#include <wrl/client.h>
#include "Engine/Math/Matrix/MatrixCalculation.h"
#include "Engine/Math/Vector/Vector4.h"
#include "Engine/DXC/FrameCount.h"

namespace DXC { class DXCom; }

namespace Colors {
	constexpr Math::Vector4 Black = { 0.0f, 0.0f, 0.0f, 1.0f };
	constexpr Math::Vector4 White = { 1.0f, 1.0f, 1.0f, 1.0f };
	constexpr Math::Vector4 Red = { 1.0f, 0.0f, 0.0f, 1.0f };
	constexpr Math::Vector4 Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
	constexpr Math::Vector4 Transparent = { 0.0f, 0.0f, 0.0f, 0.0f };
}

namespace Graphics {

	// Texture はポインタでしか持たないので前方宣言で足りる。
	// 実体を include すると DirectXTex.h(1,152行) が全ての描画オブジェクトに付いてくる
	struct Texture;

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

	/// <summary>
	/// マテリアルクラス
	/// </summary>
	class Material {
	public:
		/// <summary>
		/// マテリアルのデータ
		/// </summary>
		/// <remarks>HLSL 側の Material(Object3d.hlsli)と並び順が一致していること。追加は必ず末尾</remarks>
		struct MaterialData {
			Math::Vector4 color;
			Math::Matrix4x4 uvTransform;
			int32_t enableLighting;
			float roughness = 0.5f;			// 0=鏡面 1=完全に拡散
			float AlphaRef = 0.5f;
			float environmentCoefficient = 0.0f;	// 鏡面の環境光。オプトインなので既定は映り込ませない
			int32_t useNormalMap = 0;
			int32_t textureIndex = 0;
			int32_t normalMapIndex = 0;
			float metallic = 0.0f;			// 0=誘電体 1=金属
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
		/// <summary>CBへ最新値を書き込んでから定数バッファを返す</summary>
		/// <remarks>取得のたびに転送が走る。描画直前のバインド用であって、単なる getter ではない</remarks>
		ID3D12Resource* UploadAndGetResource();
		std::string GetPathName() const { return textureNamePath_.textureFilePath; }
		Math::Vector4 GetColor();
		Math::Vector2 GetUVScale() { return scale_; }
		Math::Vector2 GetUVTrans() { return uvTrans_; }
		int32_t GetUseNormalMap() { return materialData_.useNormalMap; }
		float GetRoughness() const { return materialData_.roughness; }
		float GetMetallic() const { return materialData_.metallic; }
		float GetEnvironment() const { return materialData_.environmentCoefficient; }

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
		/// <summary>粗さの設定(0=鏡のように鋭い 1=完全に拡散)</summary>
		void SetRoughness(float roughness);
		/// <summary>金属度の設定(0=誘電体 1=金属)</summary>
		void SetMetallic(float metallic);
		/// <summary>環境マップの設定</summary>
		void SetEnvironment(float env);

	private:

		void CopyData(uint32_t frameIndex = 0);

		DXC::DXCom* dxcommon_;

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