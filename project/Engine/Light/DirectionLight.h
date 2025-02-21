#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include "MatrixCalculation.h"


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

	void Initialize();

	void SetLightCommand(ID3D12GraphicsCommandList* commandList);

	DirectionalLight* directionLightData_ = nullptr;

	void Debug();

private:

private:

	Microsoft::WRL::ComPtr<ID3D12Resource> drectionLightResource_ = nullptr;

};