#pragma once
#include "PointLight.h"
#include <memory>

class PointLightManager {
public:
	PointLightManager() = default;
	~PointLightManager() = default;

public:

	//static PointLight* GetInstance();

	void AddPointLight();


private:

private:

	std::vector<std::unique_ptr<PointLight>> pointLights_;

};
