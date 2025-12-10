#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include "Math/Matrix/MatrixCalculation.h"


class DXCom;

/// <summary>
/// ディレクションライトのデータ
/// </summary>
struct DirectionalLight {

	Math::Vector4 color;
	Math::Vector3 direction;
	float intensity;

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

	DirectionalLight* directionLightData_ = nullptr;

	void Debug();

private:

private:

	DXCom* dxcommon_;
	Microsoft::WRL::ComPtr<ID3D12Resource> drectionLightResource_ = nullptr;

};