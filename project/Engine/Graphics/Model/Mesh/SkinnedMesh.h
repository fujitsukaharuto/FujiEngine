#pragma once
#include "Engine/Math/Vector/Vector3.h"
#include "Engine/Math/Vector/Vector2.h"
#include "Engine/Math/Vector/Vector4.h"

#include <d3d12.h>
#include <vector>
#include <wrl.h>

namespace DXC { class DXCom; }

namespace Graphics {

	/// <summary>
	/// メッシュクラス
	/// </summary>
	class SkinnedMesh {
	public:
		static constexpr uint32_t kInvalidSrvIndex = UINT32_MAX;

		/// <summary>読むときの状態。描画の頂点バッファ、BLAS 構築、反射のレイが当たった先の頂点引きで読まれる</summary>
		static constexpr D3D12_RESOURCE_STATES kReadState =
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

		SkinnedMesh();
		~SkinnedMesh();

		// 破棄時に SRV のスロットを返すので、コピーすると二重に返してしまう
		SkinnedMesh(const SkinnedMesh&) = delete;
		SkinnedMesh& operator=(const SkinnedMesh&) = delete;
		SkinnedMesh(SkinnedMesh&& other) noexcept;
		SkinnedMesh& operator=(SkinnedMesh&& other) noexcept;

		/// <summary>コンピュートシェーダー(スキニング)用のUAV/SRV生成</summary>
		void CreateUAV(size_t vertexSize);

		/// <summary>スキニングの書き込み先にする遷移を足す。読める状態だったときだけ</summary>
		void BeginWrite(std::vector<D3D12_RESOURCE_BARRIER>& barriers);
		/// <summary>書き終えて読める状態にする遷移を足す。次の BeginWrite までこのまま</summary>
		void EndWrite(std::vector<D3D12_RESOURCE_BARRIER>& barriers);

		//========================================================================*/
		//* Getter
		const D3D12_VERTEX_BUFFER_VIEW& GetSkinnedVBV() const { return skinnedVBV_; }

		// バリア遷移やComputeShader用
		ID3D12Resource* GetSkinnedResource() const { return skinnedVertexBuffer_.Get(); }
		std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> GetUavHandle() const { return uavHandle_; }
		/// <summary>スキニング後の頂点を StructuredBuffer として読む SRV の番号</summary>
		uint32_t GetSrvIndex() const { return srvIndex_; }

	private:
		void FreeDescriptors();

	private:
		DXC::DXCom* dxcommon_;

		Microsoft::WRL::ComPtr<ID3D12Resource> skinnedVertexBuffer_ = nullptr;
		D3D12_VERTEX_BUFFER_VIEW skinnedVBV_{};
		std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> uavHandle_{};
		uint32_t uavIndex_ = kInvalidSrvIndex;
		uint32_t srvIndex_ = kInvalidSrvIndex;
		bool isReadable_ = false;
	};
}
