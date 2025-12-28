#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include "Math/Matrix/MatrixCalculation.h"
#include "Engine/DX/FrameCount.h"

class DXCom;

struct PointLightData {
	Math::Vector4 color;
	Math::Vector3 position;
	float intensity;
	float radius;
	float decay;
	float pading[2];
};

/// <summary>
/// ポイントライトのクラス
/// </summary>
class PointLight {
public:
	PointLight() = default;
	~PointLight() = default;

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
	Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource_[DXC::kFrameCount_];
	PointLightData* pointLightDataGPU_[DXC::kFrameCount_];
	PointLightData pointLightData_;

	bool isOnceCopy_ = true;
};
