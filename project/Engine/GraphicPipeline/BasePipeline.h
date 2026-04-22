#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <dxcapi.h>
#include <vector>
#include <string>

#include "Engine/DX/DXCompile.h"
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"
#include <unordered_map>

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
	ID3D12RootSignature* GetRootSignature();

	/// <summary>
	/// ルートパラメータのインデックスを取得
	/// </summary>
	uint32_t GetRootIndex(const std::string& name) const;

	/// <summary>
	/// 指定した名前のルートパラメータが存在するか確認
	/// </summary>
	bool HasRootIndex(const std::string& name) const { return rootParameterMap_.contains(name); }

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
	ComPtr<ID3D12ShaderReflection> vsReflection_ = nullptr;
	ComPtr<ID3D12ShaderReflection> psReflection_ = nullptr;

	std::unordered_map<std::string, uint32_t> rootParameterMap_;

	const std::wstring kDirectoryPath_ = L"./resource/Shaders/";

};
