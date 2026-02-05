#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include "Math/Matrix/MatrixCalculation.h"
#include "Engine/DX/FrameCount.h"

class DXCom;

struct SpotLightData {
	Math::Vector4 color = { 1.0f,1.0f,1.0f,1.0f };
	Math::Vector3 position = { 0.0f,2.0f,0.0f };
	float intensity = 1.0f;
	Math::Vector3 direction = { -1.0f,-1.0f,0.0f };
	float distance = 6.0f;
	float decay = 2.0f;
	float cosAngle;
	float cosFalloffStart = 3.0f;
	float padding[1];
};

/// <summary>
/// スポットライトのクラス
/// </summary>
class SpotLight {
public:
	SpotLight() = default;
	~SpotLight() = default;

public:

	void Initialize(DXCom* pDxcom);
	void Finalize();

	/// <summary>
	/// データをコマンドリストに送る
	/// </summary>
	void SetLightCommand(ID3D12GraphicsCommandList* commandList);

	void SetIsOnceCopy(bool is) { isOnceCopy_ = is; }

	void Debug();

private:

	void CopyData(uint32_t frameIndex = 0);

private:

	DXCom* dxcommon_;
	Microsoft::WRL::ComPtr<ID3D12Resource> spotLightResource_[DXC::kFrameCount_];
	SpotLightData* spotLightDataGPU_[DXC::kFrameCount_];
	SpotLightData spotLightData_;

	bool isOnceCopy_ = true;
};
