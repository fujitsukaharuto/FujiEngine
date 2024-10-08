#pragma once
#include <wrl/client.h>
#include <string>
#include "Model.h"
#include "Camera.h"
#include "Object3dCommon.h"


class PointLight;
class SpotLight;

class Object3d {
public:
	Object3d() = default;
	~Object3d();

public:

	void Create(const std::string& fileName, Object3dCommon* common);

	void CreateSphere(Object3dCommon* common);

	void Draw();

	void SetColor(const Vector4& color);

	void SetRightDir(const Vector3& right) { directionalLightData_->direction = right; }

	void SetCamera(Camera* camera) { this->camera_ = camera; }

	/*void SetPointLight(PointLight* light) { pointLight_ = light; }

	void SetSpotLight(SpotLight* light) { spotLight_ = light; }*/

	Trans transform{};

private:

	void SetModel(const std::string& fileName);

	void CreateWVP();

	void SetWVP();

private:
	Object3dCommon* common_;
	Model* model_ = nullptr;
	PointLight* pointLight_;
	SpotLight* spotLight_;
	Camera* camera_;


	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_ = nullptr;
	TransformationMatrix* wvpDate_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_ = nullptr;
	DirectionalLight* directionalLightData_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> cameraPosResource_ = nullptr;
	CameraForGPU* cameraPosData_ = nullptr;

};
