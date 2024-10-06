#pragma once
#include <wrl/client.h>
#include "MatrixCalculation.h"
#include "Material.h"
#include "TextureManager.h"




class Sprite {
public:
	Sprite();
	~Sprite();

public:

	void Draw();



	void SetPos(const Vector3& pos);

	void SetSize(const Vector2& textureSize);

	void SetAngle(float rotate);

private:



private:


	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer_ = nullptr;

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

	std::vector<VertexDate> vertices_;
	std::vector<uint32_t> indices_;


	Microsoft::WRL::ComPtr<ID3D12Resource> transformBuffer_ = nullptr;
	Matrix4x4 matTransformData_;


	Material material_;

	Vector3 position_ = { 0,0,0 };
	Vector2 size_ = { 50,50 };
	float angle_ = 0.0f;




};
