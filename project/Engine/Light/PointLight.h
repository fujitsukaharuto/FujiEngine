#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include "Math/Matrix/MatrixCalculation.h"
#include "Engine/DX/FrameCount.h"

class DXCom;

struct PointLightData {
	Math::Vector4 color = { 1.0f,0.5f,0.0f,1.0f };
	Math::Vector3 position = { 0.0f,2.0f,0.0f };
	float intensity = 0.0f;
	float radius = 10.0f;
	float decay = 2.0f;
	float padding[2];
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

	void SetLightColor(const Math::Vector4& color);
	void SetLightPos(const Math::Vector3& pos);
	void SetAttenuationLight(float time,float intensity);

	/// <summary>
	/// データをコマンドリストに送る
	/// </summary>
	void SetLightCommand(ID3D12GraphicsCommandList* commandList);

	void SetIsOnceCopy(bool is) { isOnceCopy_ = is; }

	void Debug();

private:

	void CopyData(uint32_t frameIndex = 0);
	void AttenuationLight();

private:

	DXCom* dxcommon_;
	Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource_[DXC::kFrameCount_];
	PointLightData* pointLightDataGPU_[DXC::kFrameCount_];
	PointLightData pointLightData_;

	bool isOnceCopy_ = true;
	bool isAttenuation_ = false;
	float attenuationTime_ = 0.0f;
	float attenuationBaseTime_ = 0.0f;
	float baseIntensity_ = 0.0f;
};
