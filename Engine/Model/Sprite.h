#pragma once
#include <wrl/client.h>
#include "MatrixCalculation.h"
#include "Material.h"
#include "TextureManager.h"



class PointLight;
class SpotLight;

class Sprite {
public:
	Sprite() = default;
	~Sprite() = default;

public:

	void Load(const std::string& fileName);

	void Draw();

	void SetColor(const Vector4& color);

	void SetPos(const Vector3& pos);

	void SetSize(const Vector2& size);

	void SetAngle(float rotate);

	void SetAnchor(const Vector2& anchor);

	void SetFlipX(bool is) { isFlipX_ = is; }

	void SetFlipY(bool is) { isFlipY_ = is; }

	void SetRange(const Vector2& leftTop, const Vector2& size);

	void SetTexture(const std::string& fileName);

	/*void SetPointLight(PointLight* light) { pointLight_ = light; }

	void SetSpotLight(SpotLight* light) { spotLight_ = light; }*/

private:

	void InitializeBuffer();

	void AdjustTextureSize();

	void SetWvp();

private:


	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_ = nullptr;

	VertexDate* vData;

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

	std::vector<VertexDate> vertex_;
	std::vector<uint32_t> index_;


	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_ = nullptr;
	TransformationMatrix* wvpData_;

	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_ = nullptr;
	DirectionalLight* directionalLightData_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> cameraPosResource_ = nullptr;
	CameraForGPU* cameraPosData_ = nullptr;

	Material material_;
	std::string nowtexture;

	Vector2 anchorPoint_{ 0.5f,0.5f };
	Vector3 position_ = { 0,0,0 };
	Vector2 size_ = { 200, 200 };
	float rotate_ = 0.0f;

	bool isFlipX_ = false;
	bool isFlipY_ = false;

};
