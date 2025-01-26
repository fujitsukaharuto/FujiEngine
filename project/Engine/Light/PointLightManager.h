#pragma once
#include "PointLight.h"
#include "SpotLight.h"
#include "DirectionLight.h"
#include "RectLight.h"
#include <d3d12.h>
#include <memory>

class PointLightManager {
public:
	PointLightManager() = default;
	~PointLightManager() = default;

public:

	static PointLightManager* GetInstance();

	void Update();

	void CreateLight();
	void AddPointLight();
	void AddSpotLight();

	void SetLightCommand(ID3D12GraphicsCommandList* commandList);

	DirectionLight* GetDirectionLight() { return directionLight_.get(); }
	PointLight* GetPointLight(int num) { return pointLights_[num].get(); }
	SpotLight* GetSpotLight(int num) { return spotLights_[num].get(); }
	RectLight* GetRectLight() { return rectLight_.get(); }

private:

private:

	std::vector <std::unique_ptr<PointLight>> pointLights_;
	std::vector<std::unique_ptr<SpotLight>> spotLights_;
	std::unique_ptr<DirectionLight> directionLight_;
	std::unique_ptr<RectLight> rectLight_;

};
