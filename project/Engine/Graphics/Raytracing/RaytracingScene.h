#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "Engine/Math/Matrix/MatrixCalculation.h"

namespace DXC { class DXCom; }

namespace Graphics {

	class Model;

	/// <summary>
	/// レイトレ用の加速構造(BLAS/TLAS)を管理するクラス
	/// </summary>
	/// <remarks>
	/// レイは任意の方向へ飛ぶので、ラスタライズと違い「シーン全体」を空間検索できる木が要る。
	/// その木はアプリでは組めず、ドライバに BuildRaytracingAccelerationStructure で作らせる。
	///
	/// 二段構成になっているのは更新コストを分けるため。
	/// BLAS = 三角形そのものの木。モデルのローカル空間で1回だけ作れば使い回せる。
	/// TLAS = 何がどこにあるかの配置表。毎フレーム作り直すが、中身はインスタンスの一覧だけなので軽い。
	///
	/// 現状は静的メッシュのみが対象。スキンメッシュは頂点が毎フレーム動くのでBLASの作り直しが要り、
	/// 一緒に入れると不具合が出たときに配管の問題かスキニング連携の問題か切り分けられなくなる。
	/// パーティクルは数が多く半透明なのでレイの Yes/No と相性が悪く、そもそも入れない。
	/// </remarks>
	class RaytracingScene {
	public:
		RaytracingScene() = default;
		~RaytracingScene() = default;

		void Initialize(DXC::DXCom* pDxcom);
		void Finalize();

		/// <summary>
		/// 加速構造が使えるか。RayQuery非対応の環境では常にfalse
		/// </summary>
		bool IsAvailable() const { return isAvailable_; }

		//========================================================================*/
		//* 1フレームの流れ : BeginFrame -> AddInstance を必要なだけ -> BuildTlas

		/// <summary>このフレームのインスタンス収集を始める</summary>
		void BeginFrame();

		/// <summary>
		/// このフレームのTLASに載せるインスタンスを1つ積む
		/// </summary>
		/// <remarks>初めて見るモデルはここでBLASが構築される(同期実行なので初回だけ引っかかる)</remarks>
		/// <param name="model">モデル</param>
		/// <param name="world">ワールド行列</param>
		void AddInstance(const Model* model, const Math::Matrix4x4& world);

		/// <summary>積まれたインスタンスからTLASを構築する</summary>
		void BuildTlas();

		//========================================================================*/
		//* Getter

		/// <summary>TLASのSRV番号。未構築なら kInvalidSrvIndex</summary>
		uint32_t GetTlasSrvIndex() const { return tlasSrvIndex_; }

		/// <summary>このフレームに積まれたインスタンス数</summary>
		uint32_t GetInstanceCount() const { return static_cast<uint32_t>(instances_.size()); }

		/// <summary>BLASを構築済みのモデル数</summary>
		size_t GetBlasCount() const { return blasMap_.size(); }

		/// <summary>加速構造の状態を表示する</summary>
		void DebugGUI();

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

	private:
		DXC::DXCom* dxcommon_ = nullptr;
		bool isAvailable_ = false;

		// モデル単位でBLASを持つ。Model側にDXRを持ち込まないよう、対応付けはこちらで抱える
		std::unordered_map<const Model*, Blas> blasMap_;

		Microsoft::WRL::ComPtr<ID3D12Resource> tlasBuffer_;
		Microsoft::WRL::ComPtr<ID3D12Resource> tlasScratch_;

		// インスタンス記述子はCPUから毎フレーム書くのでUPLOADに置き、永続Mapする
		Microsoft::WRL::ComPtr<ID3D12Resource> instanceBuffer_;
		D3D12_RAYTRACING_INSTANCE_DESC* instanceMapped_ = nullptr;
		uint32_t instanceCapacity_ = 0;

		std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instances_;

		uint32_t tlasSrvIndex_ = kInvalidSrvIndex;
	};

}
