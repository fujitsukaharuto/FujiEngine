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
	void Finalize();
	void Update();

	/// <summary>
	/// カメラの取得
	/// </summary>
	/// <returns></returns>
	Camera* GetCamera()const { return camera_.get(); }

	/// <summary>
	/// Debugモードかどうかの取得
	/// </summary>
	/// <returns></returns>
	bool GetDebugMode()const { return debugMode_; }

	/// <summary>
	/// Debugモードを設定
	/// </summary>
	/// <param name="is">Debugモードにするかどうか</param>
	void SetDebugMode(bool is);

private:



private:

	std::unique_ptr<Camera> camera_ = nullptr;

	bool debugMode_ = false;

};
