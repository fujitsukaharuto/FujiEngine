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
		SkinnedMesh();
		~SkinnedMesh();

		/// <summary>コンピュートシェーダー(スキニング)用のUAV/SRV生成</summary>
		void CreateUAV(size_t vertexSize);

		//========================================================================*/
		//* Getter
		const D3D12_VERTEX_BUFFER_VIEW& GetSkinnedVBV() const { return skinnedVBV_; }

		// バリア遷移やComputeShader用
		ID3D12Resource* GetSkinnedResource() const { return skinnedVertexBuffer_.Get(); }
		std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> GetUavHandle() const { return uavHandle_; }

	private:
		DXC::DXCom* dxcommon_;

		Microsoft::WRL::ComPtr<ID3D12Resource> skinnedVertexBuffer_ = nullptr;
		D3D12_VERTEX_BUFFER_VIEW skinnedVBV_{};
		std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> uavHandle_;
	};
}