#pragma once
#include "PointLight.h"
#include "SpotLight.h"
#include <d3d12.h>
#include <memory>

class PointLightManager {
public:
	PointLightManager() = default;
	~PointLightManager() = default;

public:

	static PointLightManager* GetInstance();

	void AddPointLight();
	void AddSpotLight();

	void SetLightCommand(ID3D12GraphicsCommandList* commandList);


private:

private:

	std::vector <std::unique_ptr<PointLight>> pointLights_;
	std::vector<std::unique_ptr<SpotLight>> spotLights_;

};
