#pragma once
#include "Game/OriginObject/OriginGameObject.h"
#include "ImGuiManager/ImGuiManager.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/SRVManager.h"
#include "Engine/Camera/CameraManager.h"
#include "Engine/DX/FrameCount.h"


/// <summary>
/// SkyBox用クラス
/// </summary>
class SkyBox{
public:
	SkyBox();
	~SkyBox();

	void Initialize();
	void Update();
	void Draw();
	void DebugGUI();

	/// <summary>
	/// 行列更新
	/// </summary>
	void UpdateWVP();

	/// <summary>
	/// 必要なクラスのSetter
	/// </summary>
	void SetCommonResources(DXCom* dxcommon, SRVManager* srvManager, Camera* camera);

private:

	void ResourceCreate();
	void CreateVertex();

private:

	DXCom* dxcommon_;
	SRVManager* srvManager_;
	Camera* camera_;

	Math::Trans transform_;

	ComPtr<ID3D12Resource> vBuffer_;
	ComPtr<ID3D12Resource> iBuffer_;
	D3D12_VERTEX_BUFFER_VIEW vbView{};
	D3D12_INDEX_BUFFER_VIEW ibView{};

	std::vector<VertexDate> vertex_;
	std::vector<uint32_t> index_;

	Graphics::Material material_;

	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_[DXC::kFrameCount_];
	Math::TransformationMatrix* wvpDateGPU_[DXC::kFrameCount_];

};
