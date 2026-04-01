#include "Sprite.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/DX12Helper.h"
#include "LightManager.h"
#include "TextureManager.h"
#include "SpriteRenderer.h"
#include "Engine/Model/ModelManager.h"
#include "Engine/Light/LightManager.h"

using namespace Core;
using namespace Graphics;
using namespace Math;

Sprite::Sprite() {
	dxcommon_ = TextureManager::GetInstance()->ShareDXCom();
}

Sprite::~Sprite() {
	dxcommon_ = nullptr;
}

void Sprite::Load(const std::string& fileName) {
	material_.SetTextureNamePath(fileName);
	nowTexture = fileName;

	material_.CreateMaterial();
	material_.SetLightEnable(LightMode::kLightNone);

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		wvpResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(TransformationMatrix));
		wvpDataGPU_[i] = nullptr;
		wvpResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&wvpDataGPU_[i]));

		cameraPosResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(DirectionalLight));
		cameraPosData_[i] = nullptr;
		cameraPosResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&cameraPosData_[i]));
		cameraPosData_[i]->worldPosition = { 0.0f,0.0f,0.0f };
	}

	objIDDataResource_ = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(ObjIDData));
	objIDData_ = nullptr;
	objIDDataResource_->Map(0, nullptr, reinterpret_cast<void**>(&objIDData_));
	objIDData_->objID = -1;

	SetAnchor({ 0.5f, 0.5f });
	AdjustTextureSize();
}

void Sprite::Draw() {
	SetWvp();
	SpriteRenderer::GetInstance()->Add(this);
}

void Sprite::SetColor(const Vector4& color) {
	material_.SetColor(color);
}
void Sprite::SetPos(const Vector3& pos) {
	position_ = pos;
	SetWvp();
}
void Sprite::SetScale(const Vector2& scale) {
	scale_ = scale;
	SetWvp();
}
void Sprite::SetSize(const Vector2& size) {
	size_ = size;
	SetWvp();
}
void Sprite::SetAngle(float rotate) {
	rotate_ = rotate;
	SetWvp();
}

void Sprite::SetAnchor(const Vector2& anchor) {
	anchorPoint_ = anchor;
	SetWvp();
}

void Sprite::SetRange(const Math::Vector2& leftTop, const Math::Vector2& size) {
	// 元のテクスチャサイズを基準に、UVのスケールとオフセットを計算する
	Math::Vector2 uvScale;
	uvScale.x = size.x / defaultSize_.x;
	uvScale.y = size.y / defaultSize_.y;

	Math::Vector2 uvTrans;
	uvTrans.x = leftTop.x / defaultSize_.x;
	uvTrans.y = leftTop.y / defaultSize_.y;

	// マテリアルに計算したUV変換を適用
	material_.SetUVScale(uvScale, uvTrans);
	// スプライト自体の描画サイズも、切り抜いたサイズに合わせる
	size_ = size;
	SetWvp();
}

void Sprite::AdjustTextureSize() {
	const DirectX::TexMetadata& meta = TextureManager::GetInstance()->GetMetaData(nowTexture);

	size_ = { static_cast<float>(meta.width),static_cast<float>(meta.height) };
	defaultSize_ = size_;
	SetWvp();
}

void Sprite::SetWvp() {
	if (!wvpDataGPU_[0]) return; // 初期化前に呼ばれた場合の対策

	// 反転（フリップ）対応
	float fx = isFlipX_ ? -1.0f : 1.0f;
	float fy = isFlipY_ ? -1.0f : 1.0f;

	// アンカーポイントによるオフセット行列
	Matrix4x4 anchorMatrix = MakeTranslateMatrix({ -anchorPoint_.x, -anchorPoint_.y, 0.0f });
	// スケール（反転含む）行列
	Matrix4x4 scaleMatrix = MakeScaleMatrix({ size_.x * scale_.x * fx, size_.y * scale_.y * fy, 1.0f });
	// 回転行列
	Matrix4x4 rotateMatrix = MakeRotateZMatrix(rotate_);
	// 平行移動（配置座標）行列
	Matrix4x4 translateMatrix = MakeTranslateMatrix(position_);

	Matrix4x4 worldMatrix = Multiply(Multiply(Multiply(anchorMatrix, scaleMatrix), rotateMatrix), translateMatrix);
	Matrix4x4 viewMatrix = MakeIdentity4x4();
	Matrix4x4 projectionMatrix = MakeOrthographicMatrix(0.0f, 0.0f, float(MyWin::kWindowWidth), float(MyWin::kWindowHeight), 0.0f, 100.0f);

	Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));

	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	wvpDataGPU_[frameIndex]->World = worldMatrix;
	wvpDataGPU_[frameIndex]->WVP = worldViewProjectionMatrix;
	wvpDataGPU_[frameIndex]->WorldInverseTransPose = Transpose(Inverse(wvpDataGPU_[frameIndex]->World));
}

// --- Rendererへ渡すためのGetter ---
D3D12_GPU_VIRTUAL_ADDRESS Sprite::GetWvpGPUAddress(uint32_t frameIndex) const {
	return wvpResource_[frameIndex]->GetGPUVirtualAddress();
}
D3D12_GPU_VIRTUAL_ADDRESS Sprite::GetMaterialGPUAddress() {
	return material_.GetMaterialResource()->GetGPUVirtualAddress();
}
D3D12_GPU_DESCRIPTOR_HANDLE Sprite::GetTextureSRV() {
	return material_.GetTexture()->gpuHandle;
}