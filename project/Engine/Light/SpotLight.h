#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include "Math/Matrix/MatrixCalculation.h"

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
	float pading[1];
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

	SpotLightData* spotLightData_ = nullptr;

	void Debug();

private:

private:

	DXCom* dxcommon_;
	Microsoft::WRL::ComPtr<ID3D12Resource> spotLightResource_ = nullptr;

};
