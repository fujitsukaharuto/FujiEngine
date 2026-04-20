#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include "Math/Matrix/MatrixCalculation.h"
#include "Engine/DX/FrameCount.h"


class DXCom;

/// <summary>
/// ディレクションライトのデータ
/// </summary>
struct DirectionalLight {
	Math::Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
	Math::Vector3 direction = { 0.0f, -1.0f, 0.0f };
	float intensity = 1.0f;

	// 互換性のためのセッター
	void SetLightDirection(const Math::Vector3& dir) { direction = Math::Vector3::Normalize(dir); }
	void SetLightIntensity(float inst) { intensity = inst; }
	void SetColor(const Math::Vector4& col) { color = col; }
};

/// <summary>
/// ディレクションライトのクラス
/// </summary>
class DirectionLight {
public:
	DirectionLight() = default;
	~DirectionLight() = default;

public:

	void Initialize(DXCom* pDxcom);
	void Finalize();

	/// <summary>
	/// データをコマンドリストに送る
	/// </summary>
	void SetLightCommand(ID3D12GraphicsCommandList* commandList);

	void SetLightColor(const Math::Vector4& color);
	void SetLightDirection(const Math::Vector3& direction);
	void SetLightIntensity(float intensity);

	void SetIsOnceCopy(bool is) { isOnceCopy_ = is; }

	void Debug();

private:

	void CopyData(uint32_t frameIndex = 0);

private:

	DXCom* dxcommon_;
	Microsoft::WRL::ComPtr<ID3D12Resource> directionLightResource_[DXC::kFrameCount_];
	DirectionalLight* directionLightDataGPU_[DXC::kFrameCount_];
	DirectionalLight directionLightData_;

	bool isOnceCopy_ = true;
};