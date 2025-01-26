#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include "MatrixCalculation.h"


class DirectionLight {
public:
	DirectionLight() = default;
	~DirectionLight() = default;

public:

	void Initialize();


	void SetLightCommand(ID3D12GraphicsCommandList* commandList);

	DirectionalLight* directionLightData_ = nullptr;

#ifdef _DEBUG
	void Debug();
#endif // _DEBUG

private:



private:

	Microsoft::WRL::ComPtr<ID3D12Resource> drectionLightResource_ = nullptr;


};
