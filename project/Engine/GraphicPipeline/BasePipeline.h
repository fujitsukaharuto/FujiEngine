#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <dxcapi.h>
#include <vector>
#include <string>

#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"

using namespace Microsoft::WRL;

/// <summary>
/// 合成モード
/// </summary>
enum class BlendType {
	ALPHA,
	ADD,
	SUBTRACT,
	SCREEN,
	MULTIPLY,
	SOFT_ADD,
	PREMULTIPLIED_ALPHA
};

class DXCom;

/// <summary>
/// パイプラインの基底クラス
/// </summary>
class BasePipeline {
public:
	BasePipeline() = default;
	~BasePipeline();

	void Initialize(DXCom* pDxcom);

	/// <summary>
	/// グラフィックスパイプラインステートを設定
	/// </summary>
	void SetPipelineState();

	/// <summary>
	/// コンピュートシェーダ用のパイプラインステートを設定
	/// </summary>
	void SetPipelineCSState(uint32_t index = 0);

	/// <summary>
	/// パイプラインを生成する
	/// </summary>
	void CreatePipe();

private:

	/// <summary>
	/// ルートシグネチャを生成
	/// </summary>
	virtual void CreateRootSignature(ID3D12Device* device);

	/// <summary>
	/// PSOを生成
	/// </summary>
	virtual void CreatePSO(ID3D12Device* device);

protected:

	DXCom* dxcommon_;

	ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
	ComPtr<ID3D12PipelineState> pso_ = nullptr;

	ComPtr<IDxcBlob> vs = nullptr;
	ComPtr<IDxcBlob> ps = nullptr;

	const std::wstring kDirectoryPath_ = L"./resource/Shaders/";

};
