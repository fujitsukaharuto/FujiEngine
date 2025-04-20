#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include "Math/Matrix/MatrixCalculation.h"


class DXCom;

struct DirectionalLight {

	Vector4 color;
	Vector3 direction;
	float intensity;

};


class DirectionLight {
public:
	DirectionLight() = default;
	~DirectionLight() = default;

public:

	void Initialize(DXCom* pDxcom);
	void Finalize();

	void SetLightCommand(ID3D12GraphicsCommandList* commandList);

	DirectionalLight* directionLightData_ = nullptr;

	void Debug();

private:

private:

	DXCom* dxcommon_;
	Microsoft::WRL::ComPtr<ID3D12Resource> drectionLightResource_ = nullptr;

};