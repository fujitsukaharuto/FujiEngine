#pragma once
#include <wrl/client.h>
#include "MatrixCalculation.h"
#include "Material.h"
#include "TextureManager.h"




class Sprite {
public:
	Sprite() = default;
	~Sprite() = default;

public:

	void Load(const std::string& fileName);

	void Draw();

	void SetPos(const Vector3& pos);

	void SetSize(const Vector2& size);

	void SetAngle(float rotate);

private:

	void InitializeBuffer();

	void SetWvp();

private:


	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_ = nullptr;

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

	std::vector<VertexDate> vertex_;
	std::vector<uint32_t> index_;


	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_ = nullptr;
	TransformationMatrix* wvpData_;

	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_ = nullptr;
	DirectionalLight* directionalLightData_ = nullptr;

	Material material_;

	Vector3 position_ = { 0,0,0 };
	Vector2 size_ = { 200, 200 };
	float rotate_ = 0.0f;

};
