#pragma once
#include <wrl/client.h>
#include <string>
#include "Model.h"


class Object3d {
public:
	Object3d() = default;
	~Object3d();

public:

	void Create(const std::string& fileName);

	void CreateSphere();

	void Draw();

	void SetColor(const Vector4& color);

	Trans transform{};

private:

	void SetModel(const std::string& fileName);

	void CreateWVP();

	void SetWVP();

private:
	Model* model_ = nullptr;


	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_ = nullptr;
	TransformationMatrix* wvpDate_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_ = nullptr;
	DirectionalLight* directionalLightData_ = nullptr;

};
