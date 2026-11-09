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

	// 実体を include すると DirectXTex.h(1,152行) が全ての描画オブジェクトに付いてくる
	struct Texture;

	/// <summary>
	/// Textureのファイルパス
	/// </summary>
	struct MaterialDataPath {
		std::string textureFilePath;
		std::string normalFilePath;
		// 以下は glTF のマテリアルの値。ApplyModelMaterials で当てたときだけ使う
		/// <summary>モデルが法線マップを持っているか</summary>
		bool hasNormalMap = false;
		/// <summary>メタリック・ラフネスのテクスチャ (G=粗さ B=金属)。空なら無し</summary>
		std::string metallicRoughnessFilePath;
		/// <summary>テクスチャに掛ける色 (glTF の baseColorFactor)</summary>
		Math::Vector4 baseColorFactor = { 1.0f,1.0f,1.0f,1.0f };
		float metallicFactor = 1.0f;
		float roughnessFactor = 1.0f;
		/// <summary>抜きのしきい値。抜かない素材(glTF の OPAQUE)は0</summary>
		float alphaCutoff = 0.0f;
		/// <summary>両面を描く素材</summary>
		bool isTwoSided = false;
		/// <summary>発光色 (glTF の emissiveFactor × emissive_strength)</summary>
		Math::Vector3 emissiveFactor = { 0.0f,0.0f,0.0f };
		/// <summary>光る所と色のテクスチャ。空なら無し</summary>
		std::string emissiveFilePath;
		/// <summary>物陰の暗さのテクスチャ (R を使う)。空なら無し</summary>
		std::string occlusionFilePath;
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
			Math::Vector3 emissive{};		// 自分で光る色。テクスチャの色に掛けて足す。1を超えるとブルームで光がにじむ
			float rimPower = 3.0f;			// リムライトの細さ。大きいほど輪郭のきわだけが光る
			Math::Vector3 rimColor{};		// リムライトの色。黒なら掛からない
			float pad0 = 0.0f;
			int32_t metallicRoughnessIndex = 0;		// メタリック・ラフネスのテクスチャ (G=粗さ B=金属)
			int32_t useMetallicRoughnessMap = 0;	// 0以外なら roughness と metallic にテクスチャの値を掛ける
			int32_t isTwoSided = 0;					// 0以外なら裏から見た面は法線を裏返す
			float pad1 = 0.0f;
			int32_t emissiveMapIndex = 0;			// 光る所と色のテクスチャ
			int32_t useEmissiveMap = 0;				// 0以外なら emissive にこれを掛ける。0なら色のテクスチャ
			int32_t occlusionMapIndex = 0;			// 物陰の暗さのテクスチャ (R)
			int32_t useOcclusionMap = 0;			// 0以外ならアンビエントにテクスチャの R を掛ける
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
		/// <summary>シェーダの gTextures[] でこのテクスチャを引く番号</summary>
		uint32_t GetTextureSrvIndex() const;
		/// <summary>発光に掛けるテクスチャの SRV の番号。無ければ色のテクスチャ</summary>
		uint32_t GetEmissiveTextureSrvIndex() const;
		/// <summary>テクスチャに抜きがあるか</summary>
		bool IsCutout() const;
		float GetAlphaRef() const { return materialData_.AlphaRef; }
		/// <summary>CBへ最新値を書き込んでから定数バッファを返す</summary>
		/// <remarks>取得のたびに転送が走る。描画直前のバインド用であって、単なる getter ではない</remarks>
		ID3D12Resource* UploadAndGetResource();
		std::string GetPathName() const { return textureNamePath_.textureFilePath; }
		Math::Vector4 GetColor() const;
		Math::Vector2 GetUVScale() { return scale_; }
		Math::Vector2 GetUVTrans() { return uvTrans_; }
		int32_t GetUseNormalMap() { return materialData_.useNormalMap; }
		float GetRoughness() const { return materialData_.roughness; }
		float GetMetallic() const { return materialData_.metallic; }
		const Math::Vector3& GetEmissive() const { return materialData_.emissive; }
		const Math::Vector3& GetRimColor() const { return materialData_.rimColor; }
		float GetRimPower() const { return materialData_.rimPower; }
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
		/// <summary>メタリック・ラフネスのテクスチャ。roughness と metallic は倍率になる</summary>
		void SetMetallicRoughnessMap(const std::string& name);
		/// <summary>両面を描く素材か</summary>
		void SetTwoSided(bool isTwoSided) { materialData_.isTwoSided = isTwoSided ? 1 : 0; }
		/// <summary>光る所と色のテクスチャ。emissive に掛ける</summary>
		void SetEmissiveMap(const std::string& name);
		/// <summary>物陰の暗さのテクスチャ (R)。アンビエントにだけ掛かる</summary>
		void SetOcclusionMap(const std::string& name);
		/// <summary>ライトモードの設定</summary>
		void SetLightEnable(LightMode mode);
		/// <summary>粗さの設定(0=鏡のように鋭い 1=完全に拡散)</summary>
		void SetRoughness(float roughness);
		/// <summary>金属度の設定(0=誘電体 1=金属)</summary>
		void SetMetallic(float metallic);
		/// <summary>自分で光る色の設定。ライトが当たっていなくてもこの色で見える</summary>
		void SetEmissive(const Math::Vector3& emissive) { materialData_.emissive = emissive; }
		/// <summary>リムライト(輪郭を光らせて背景から浮かせる)の設定。色が黒なら掛からない</summary>
		/// <param name="power">細さ。大きいほど輪郭のきわだけが光る</param>
		void SetRim(const Math::Vector3& color, float power) { materialData_.rimColor = color; materialData_.rimPower = power; }
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
		Texture* metallicRoughnessMap_ = nullptr;
		Texture* emissiveMap_ = nullptr;
		Texture* occlusionMap_ = nullptr;
		Texture* normalMap_ = nullptr;
		MaterialDataPath textureNamePath_;
	};
}