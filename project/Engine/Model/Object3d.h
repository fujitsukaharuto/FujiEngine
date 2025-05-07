#pragma once
#include <wrl/client.h>
#include <string>
#include "Model.h"
#include "Camera.h"
#include "Object3dCommon.h"

class DXCom;
class LightManager;
class PointLight;
class SpotLight;

class Object3d {
public:
	Object3d();
	~Object3d();

public:

	void Create(const std::string& fileName);

	void CreateSphere();

	void Draw(Material* mate = nullptr);

	void AnimeDraw();

	Matrix4x4 GetWorldMat() const;

	Vector3 GetWorldPos()const;

	void UpdateWVP() { SetWVP(); }

	void DebugGUI();

	void SetColor(const Vector4& color);

	void SetUVScale(const Vector2& scale, const Vector2& uvTrans);

	void SetCamera(Camera* camera) { this->camera_ = camera; }

	void SetParent(Object3d* parent) { parent_ = parent; }

	void SetCameraParent(bool is) { isCameraParent_ = is; }

	void SetTexture(const std::string& name);

	void SetBillboardMat(const Matrix4x4& mat) { billboardMatrix_ = mat; }

	void SetLightEnable(LightMode mode);

	void SetModel(const std::string& fileName);

	Trans transform{};

private:

	void CreateWVP();

	void SetWVP();

	void SetBillboardWVP();

private:
	Object3dCommon* common_;
	std::unique_ptr<Model> model_ = nullptr;

	DXCom* dxcommon_;
	LightManager* lightManager_;
	Camera* camera_;
	Object3d* parent_ = nullptr;

	bool isCameraParent_ = false;

	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_ = nullptr;
	TransformationMatrix* wvpDate_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> cameraPosResource_ = nullptr;
	CameraForGPU* cameraPosData_ = nullptr;

	Matrix4x4 billboardMatrix_;
	std::string nowTextureName;
};