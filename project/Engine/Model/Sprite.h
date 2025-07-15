#pragma once
#include <wrl/client.h>
#include "Math/Matrix/MatrixCalculation.h"
#include "Material.h"
#include "Object3d.h"


// Pipe別にしたい

class DXCom;
class PointLight;
class SpotLight;

class Sprite {
public:
	Sprite();
	~Sprite();

public:

	/// <summary>
	/// Textureのセット
	/// </summary>
	void Load(const std::string& fileName);

	void Draw();

	void SetColor(const Vector4& color);

	void SetPos(const Vector3& pos);

	void SetScale(const Vector2& scale);

	void SetSize(const Vector2& size);

	void SetAngle(float rotate);

	/// <summary>
	/// スプライトの基準点
	/// </summary>
	void SetAnchor(const Vector2& anchor);

	/// <summary>
	/// 左右反転
	/// </summary>
	void SetFlipX(bool is) { isFlipX_ = is; }

	/// <summary>
	/// 上下反転
	/// </summary>
	void SetFlipY(bool is) { isFlipY_ = is; }

	/// <summary>
	/// スプライトの描画範囲指定
	/// </summary>
	void SetRange(const Vector2& leftTop, const Vector2& size);

	Vector2 GetDefaultSize() const { return defaultSize_; }

	/*void SetPointLight(PointLight* light) { pointLight_ = light; }

	void SetSpotLight(SpotLight* light) { spotLight_ = light; }*/

private:

	void InitializeBuffer();

	void AdjustTextureSize();

	void SetWvp();

private:

	DXCom* dxcommon_;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_ = nullptr;

	VertexDate* vData;

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

	std::vector<VertexDate> vertex_;
	std::vector<uint32_t> index_;


	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_ = nullptr;
	TransformationMatrix* wvpData_;

	Microsoft::WRL::ComPtr<ID3D12Resource> cameraPosResource_ = nullptr;
	CameraForGPU* cameraPosData_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> objIDDataResource_ = nullptr;
	ObjIDData* objIDData_ = nullptr;

	Material material_;
	std::string nowtexture;

	Vector2 anchorPoint_{ 0.5f,0.5f };
	Vector3 position_ = { 0,0,0 };
	Vector2 defaultSize_ = {};
	Vector2 size_ = { 200, 200 };
	Vector2 scale_ = { 1.0f,1.0f };
	float rotate_ = 0.0f;

	bool isFlipX_ = false;
	bool isFlipY_ = false;

};
