#pragma once
#include "DirectionLight.h"
#include "PointLight.h"
#include "SpotLight.h"
#include <d3d12.h>
#include <memory>

class DXCom;

class LightManager {
public:
	LightManager() = default;
	~LightManager() = default;

public:

	void Initialize(DXCom* pDxcom);
	void Finalize();
	void Update();

	void CreateLight();
	void AddPointLight();
	void AddSpotLight();

	void SetLightCommand(ID3D12GraphicsCommandList* commandList);

	PointLight* GetPointLight(int num) { return pointLights_[num].get(); }
	SpotLight* GetSpotLight(int num) { return spotLights_[num].get(); }

	void DebugGUI();

private:

private:

	DXCom* dxcommon_;
	std::unique_ptr<DirectionLight> directionLight_;
	std::vector <std::unique_ptr<PointLight>> pointLights_;
	std::vector<std::unique_ptr<SpotLight>> spotLights_;

};
