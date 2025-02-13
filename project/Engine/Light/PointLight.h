#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include "MatrixCalculation.h"

struct PointLightData {
	Vector4 color;
	Vector3 position;
	float intensity;
	float radius;
	float decay;
	float pading[2];
};

class PointLight {
public:
	PointLight() = default;
	~PointLight() = default;

public:

	void Initialize();


	void SetLightCommand(ID3D12GraphicsCommandList* commandList);

	PointLightData* pointLightData_ = nullptr;

	void Debug();

private:

private:

	Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource_ = nullptr;

};
