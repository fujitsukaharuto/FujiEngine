#pragma once
#include <memory>
#include "Camera.h"

class DXCom;

/// <summary>
/// カメラ管理クラス
/// </summary>
class CameraManager {
public:
	CameraManager() = default;
	~CameraManager() = default;

public:

	static CameraManager* GetInstance();
	void Initialize(DXCom* pDXCom);
	void Finalize();
	void Update();


	//========================================================================*/
	//* Getter

	/// <summary>
	/// カメラの取得
	/// </summary>
	/// <returns>Camera</returns>
	Camera* GetCamera()const { return camera_.get(); }

	/// <summary>
	/// Debugモードかどうかの取得
	/// </summary>
	/// <returns>bool</returns>
	bool GetDebugMode()const { return debugMode_; }

	//========================================================================*/
	//* Setter

	/// <summary>
	/// Debugモードを設定
	/// </summary>
	/// <param name="is">Debugモードにするかどうか</param>
	void SetDebugMode(bool is);

	void DebugGUI();

private:



private:

	std::unique_ptr<Camera> camera_ = nullptr;

	bool debugMode_ = false;

};
