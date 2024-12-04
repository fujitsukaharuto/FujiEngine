#pragma once
#include <memory>
#include "Camera.h"




class CameraManager {
public:
	CameraManager() = default;
	~CameraManager() = default;

public:

	static CameraManager* GetInstance();

	void Initialize();

	void Update();

	Camera* GetCamera()const { return camera_.get(); }

	bool GetDebugMode()const { return debugMode_; }

	void SetDebugMode(bool is);

private:



private:

	std::unique_ptr<Camera> camera_ = nullptr;

	bool debugMode_ = false;

};
