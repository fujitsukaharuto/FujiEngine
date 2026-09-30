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
		void AddInstance(const Model* model, const Math::Matrix4x4& world);

		/// <summary>スキンメッシュのインスタンスを1つ積む</summary>
		/// <remarks>頂点が毎フレーム動くのでBLASも毎フレーム作り直す。スキニングより後に呼ぶこと</remarks>
		/// <param name="key">インスタンスを識別するもの。BLASの使い回しに使う。破棄されたものと同じアドレスが来てもよい(大きさを毎回確かめる)</param>
		/// <param name="model">モデル(インデックスバッファの取得用)</param>
		/// <param name="skinnedMeshes">スキニング後のメッシュ</param>
		/// <param name="world">ワールド行列</param>
		void AddSkinnedInstance(const RenderObject* key, const Model* model,
			const std::vector<SkinnedMesh>& skinnedMeshes, const Math::Matrix4x4& world);

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
		};

		/// <summary>GPUが使い終わるのを待ってから捨てるBLAS</summary>
		struct RetiredBlas {
			SkinnedBlas blas;
			uint64_t retiredFrame = 0;
		};

		/// <summary>このフレーム数だけ経てば、そのフレームに積んだGPUの処理は終わっている</summary>
		static constexpr uint64_t kReleaseDelayFrames = DXC::kFrameCount_ + 1;

		/// <summary>来なくなったスキンメッシュのBLASと、待ち終えた古いBLASを捨てる</summary>
		void ReleaseUnusedSkinnedBlas();

		// Model側にDXRを持ち込まないよう、対応付けはこちらで抱える
		std::unordered_map<const Model*, Blas> blasMap_;

		// スキンメッシュはポーズが個体ごとに違うので、モデルではなくインスタンス単位で持つ。
		// キーはアドレスなので、破棄されたオブジェクトの跡に別のオブジェクトが来ることがある
		std::unordered_map<const RenderObject*, SkinnedBlas> skinnedBlasMap_;
		std::vector<RetiredBlas> retiredBlas_;
		uint64_t frameNumber_ = 0;

		// 構築を描画と同じQueueに積んでいるので1つで足りる。別Queueへ移すならフレーム数ぶん要る
		Microsoft::WRL::ComPtr<ID3D12Resource> tlasBuffer_;
		Microsoft::WRL::ComPtr<ID3D12Resource> tlasScratch_;

		// CPUが毎フレーム書くのでフレーム数ぶん持つ。1枚だと前フレームの構築中に上書きする
		Microsoft::WRL::ComPtr<ID3D12Resource> instanceBuffer_[DXC::kFrameCount_];
		D3D12_RAYTRACING_INSTANCE_DESC* instanceMapped_[DXC::kFrameCount_] = {};
		uint32_t instanceCapacity_ = 0;

		std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instances_;

		uint32_t tlasSrvIndex_ = kInvalidSrvIndex;
	};

}
