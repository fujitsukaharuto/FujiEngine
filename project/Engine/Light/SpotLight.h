#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include "Math/Matrix/MatrixCalculation.h"
#include "Engine/DX/FrameCount.h"

class DXCom;

struct SpotLightData {
	Math::Vector4 color;
	Math::Vector3 position;
	float intensity;
	Math::Vector3 direction;
	float distance;
	float decay;
	float cosAngle;
	float cosFalloffStart;
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
