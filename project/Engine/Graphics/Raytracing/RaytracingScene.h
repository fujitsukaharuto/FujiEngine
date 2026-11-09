#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "Engine/DXC/FrameCount.h"
#include "Engine/Math/Matrix/MatrixCalculation.h"

namespace DXC { class DXCom; }

namespace Graphics {

	class Model;
	class SkinnedMesh;
	class RenderObject;

	/// <summary>
	/// レイが当たった物の見た目。反射に映す色を出すのに使う
	/// </summary>
	/// <remarks>HLSL 側 (Common/RayTracedReflection.hlsli) の RayInstance と並びが一致していること</remarks>
	struct RayInstanceData {
		Math::Vector3 color = { 1.0f,1.0f,1.0f };	// マテリアルの色
		uint32_t textureIndex = 0;
		Math::Vector3 emissive{};
		// 以下の3つは RaytracingScene がメッシュから入れる。呼び出し側は触らなくてよい
		uint32_t vertexBuffer = 0;	// 頂点の SRV の番号
		uint32_t indexBuffer = 0;	// インデックスの SRV の番号
		uint32_t flags = 0;			// RayInstanceFlag
		Math::Vector2 uvScale = { 1.0f,1.0f };
		Math::Vector2 uvOffset{};
		float roughness = 1.0f;		// 映り込みを付けていないマテリアルは1
		float metallic = 0.0f;
		uint32_t emissiveTextureIndex = 0;	// 発光に掛けるテクスチャ。無ければ色のテクスチャ
		float opacity = 1.0f;		// 1未満は半透明
		uint32_t pad[2] = {};
	};
	static_assert(sizeof(RayInstanceData) == 80, "RayInstanceData の大きさが RayTracedReflection.hlsli の RayInstance と一致していない");

	/// <remarks>HLSL 側 (Common/RayTracedReflection.hlsli) の kRayInstance* と同じ値であること</remarks>
	enum RayInstanceFlag : uint32_t {
		kRayInstanceHasGeometry = 1u << 0,	// 頂点とインデックスを引ける(UV が分かる)
		kRayInstanceHasNormal = 1u << 1,	// 頂点の法線が使える
		kRayInstanceCutout = 1u << 2,		// テクスチャに抜きがある
	};
	/// <summary>RayInstanceData::flags の、抜きのしきい値 (8bit) を置く位置</summary>
	/// <remarks>HLSL 側 (Common/RayInstance.hlsli) の kRayInstanceAlphaRefShift と同じ値であること</remarks>
	constexpr uint32_t kRayInstanceAlphaRefShift = 8;

	/// <summary>
	/// レイトレ用の加速構造(BLAS/TLAS)を管理するクラス
	/// </summary>
	/// <remarks>対象は静的メッシュのみ。スキンメッシュとパーティクルは含めない</remarks>
	class RaytracingScene {
	public:
		RaytracingScene() = default;
		~RaytracingScene() = default;

		void Initialize(DXC::DXCom* pDxcom);
		void Finalize();

		/// <summary>RayQueryが使えるか。使えなければ以降の処理は全て空振りする</summary>
		bool IsAvailable() const { return isAvailable_; }

		//========================================================================*/
		//* 1フレームの流れ : BeginFrame -> AddInstance -> BuildTlas

		/// <summary>このフレームのインスタンス収集を始める</summary>
		void BeginFrame();

		/// <summary>TLASに載せるインスタンスを1つ積む</summary>
		/// <remarks>初めて見るモデルはここでBLASを同期構築するので、初回だけスタールする</remarks>
		/// <param name="model">モデル</param>
		/// <param name="world">ワールド行列</param>
		/// <param name="looks">メッシュごとの見た目。モデルのメッシュと同じ並び。nullptr なら見た目を集めない(当たった物の色を映さないフレーム)</param>
		/// <param name="isTranslucent">半透明か。影のレイには当たらなくなる</param>
		/// <param name="hasCutout">抜きのあるテクスチャを使うか (looks が要る)</param>
		/// <param name="isStatic">動かない配置物か</param>
		void AddInstance(const Model* model, const Math::Matrix4x4& world, const std::vector<RayInstanceData>* looks, bool isTranslucent,
			bool hasCutout = false, bool isStatic = false);

		/// <summary>スキンメッシュのインスタンスを1つ積む</summary>
		/// <remarks>頂点が毎フレーム動くのでBLASも毎フレーム作り直す。スキニングより後に呼ぶこと</remarks>
		/// <param name="key">インスタンスを識別するもの。BLASの使い回しに使う。破棄されたものと同じアドレスが来てもよい(大きさを毎回確かめる)</param>
		/// <param name="model">モデル(インデックスバッファの取得用)</param>
		/// <param name="skinnedMeshes">スキニング後のメッシュ</param>
		/// <param name="world">ワールド行列</param>
		/// <param name="looks">メッシュごとの見た目。モデルのメッシュと同じ並び。nullptr なら見た目を集めない</param>
		/// <param name="isTranslucent">半透明か。影のレイには当たらなくなる</param>
		/// <param name="hasCutout">抜きのあるテクスチャを使うか (looks が要る)</param>
		void AddSkinnedInstance(const RenderObject* key, const Model* model,
			const std::vector<SkinnedMesh>& skinnedMeshes, const Math::Matrix4x4& world,
			const std::vector<RayInstanceData>* looks, bool isTranslucent, bool hasCutout = false);

		/// <summary>積まれたインスタンスからTLASを構築する</summary>
		void BuildTlas();

		/// <summary>構築済みのBLASを全て捨てる</summary>
		/// <remarks>モデル再読み込み時に呼ぶ。呼ばないと影が古い形状のまま残る</remarks>
		void InvalidateAllBlas();

		/// <summary>加速構造の状態を表示する</summary>
		void DebugGUI();

		//========================================================================*/
		//* Getter
		uint32_t GetTlasSrvIndex() const { return tlasSrvIndex_; }
		/// <summary>このフレームの「当たった物の見た目」の SRV。まだ無ければ kInvalidSrvIndex</summary>
		uint32_t GetInstanceDataSrvIndex() const;
		uint32_t GetInstanceCount() const { return static_cast<uint32_t>(instances_.size()); }
		size_t GetBlasCount() const { return blasMap_.size(); }

	public:
		static constexpr uint32_t kInvalidSrvIndex = UINT32_MAX;

	private:

		/// <summary>1モデルぶんのBLAS</summary>
		struct Blas {
			Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
			D3D12_GPU_VIRTUAL_ADDRESS address = 0;
		};

		/// <summary>未構築ならBLASを作る。作れなければ nullptr</summary>
		const Blas* EnsureBlas(const Model* model);

		/// <summary>インスタンス数ぶんのTLAS用バッファを確保し直す</summary>
		void EnsureTlasCapacity(uint32_t instanceCount);

		/// <summary>「当たった物の見た目」のバッファを、その個数が入る大きさに確保し直す</summary>
		void EnsureInstanceDataCapacity(uint32_t count);

		/// <summary>TLAS構築コマンドを指定のリストへ積む</summary>
		void RecordTlasBuild(ID3D12GraphicsCommandList4* list, uint32_t instanceCount, uint32_t frameIndex);

	private:
		DXC::DXCom* dxcommon_ = nullptr;
		bool isAvailable_ = false;

		/// <summary>スキンメッシュ1体ぶんのBLAS</summary>
		/// <remarks>毎フレーム作り直すのでスクラッチも持ち続ける</remarks>
		struct SkinnedBlas {
			Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
			Microsoft::WRL::ComPtr<ID3D12Resource> scratch;
			D3D12_GPU_VIRTUAL_ADDRESS address = 0;
			UINT64 bufferSize = 0;
			UINT64 scratchSize = 0;
			/// <summary>最後に積んだフレーム。しばらく来なければ持ち主が消えたとみなして捨てる</summary>
			uint64_t lastUsedFrame = 0;
			/// <summary>組んだときの形状(頂点とインデックスのバッファと個数)。同じなら更新で済ませられる</summary>
			uint64_t signature = 0;
			/// <summary>組み直してから更新で済ませた回数。更新を重ねると木が粗くなるので、ときどき組み直す</summary>
			uint32_t refitCount = 0;
		};

		/// <summary>GPUが使い終わるのを待ってから捨てるBLAS</summary>
		struct RetiredBlas {
			SkinnedBlas blas;
			uint64_t retiredFrame = 0;
		};

		/// <summary>このフレーム数だけ経てば、そのフレームに積んだGPUの処理は終わっている</summary>
		static constexpr uint64_t kReleaseDelayFrames = DXC::kFrameCount_ + 1;

		/// <summary>スキンメッシュのBLASを、組み直さずに更新で済ませる回数の上限</summary>
		static constexpr uint32_t kMaxSkinnedRefits = 120;

		/// <summary>InstanceMask。HLSL 側 (RayTracedShadow.hlsli の kRayMask*) と同じ値であること</summary>
		static constexpr uint8_t kInstanceMaskOpaque = 0x01;
		static constexpr uint8_t kInstanceMaskTranslucent = 0x02;
		// 動かない配置物にだけ足す
		static constexpr uint8_t kInstanceMaskStaticOpaque = 0x04;
		static constexpr uint8_t kInstanceMaskStaticTranslucent = 0x08;

		/// <summary>来なくなったスキンメッシュのBLASと、待ち終えた古いBLASを捨てる</summary>
		void ReleaseUnusedSkinnedBlas();

		// Model側にDXRを持ち込まないよう、対応付けはこちらで抱える
		std::unordered_map<const Model*, Blas> blasMap_;

		// スキンメッシュはポーズが個体ごとに違うので、モデルではなくインスタンス単位で持つ。
		// キーはアドレスなので、破棄されたオブジェクトの跡に別のオブジェクトが来ることがある
		std::unordered_map<const RenderObject*, SkinnedBlas> skinnedBlasMap_;
		std::vector<RetiredBlas> retiredBlas_;
		uint64_t frameNumber_ = 0;
		// TLAS を組む前に BLAS の構築を待つ必要があるか
		bool hasPendingBlasBuild_ = false;

		// 構築を描画と同じQueueに積んでいるので1つで足りる。別Queueへ移すならフレーム数ぶん要る
		Microsoft::WRL::ComPtr<ID3D12Resource> tlasBuffer_;
		Microsoft::WRL::ComPtr<ID3D12Resource> tlasScratch_;

		// CPUが毎フレーム書くのでフレーム数ぶん持つ。1枚だと前フレームの構築中に上書きする
		Microsoft::WRL::ComPtr<ID3D12Resource> instanceBuffer_[DXC::kFrameCount_];
		D3D12_RAYTRACING_INSTANCE_DESC* instanceMapped_[DXC::kFrameCount_] = {};
		uint32_t instanceCapacity_ = 0;

		std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instances_;

		// メッシュごとに1つ。インスタンスの InstanceID が、そのインスタンスの先頭の位置を指す
		std::vector<RayInstanceData> instanceData_;
		Microsoft::WRL::ComPtr<ID3D12Resource> instanceDataBuffer_[DXC::kFrameCount_];
		RayInstanceData* instanceDataMapped_[DXC::kFrameCount_] = {};
		uint32_t instanceDataSrvIndex_[DXC::kFrameCount_] = {};
		uint32_t instanceDataCapacity_ = 0;

		uint32_t tlasSrvIndex_ = kInvalidSrvIndex;
	};

}
