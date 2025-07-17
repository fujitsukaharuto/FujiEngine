#include "Sprite.h"
#include "Engine/DX/DXCom.h"
#include "LightManager.h"
#include "TextureManager.h"
#include "Engine/Model/ModelManager.h"
#include "Engine/Light/LightManager.h"

Sprite::Sprite() {
	dxcommon_ = TextureManager::GetInstance()->ShareDXCom();
}

Sprite::~Sprite() {
	dxcommon_ = nullptr;
}

void Sprite::Load(const std::string& fileName) {
	material_.SetTextureNamePath(fileName);
	nowtexture = fileName;
	InitializeBuffer();
	SetAnchor({ 0.5f, 0.5f });
	AdjustTextureSize();
}

void Sprite::Draw() {
	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	
	dxcommon_->GetDXCommand()->SetViewAndscissor();
	dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Sprite);
	dxcommon_->GetDXCommand()->GetList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	cList->IASetVertexBuffers(0, 1, &vertexBufferView_);
	cList->IASetIndexBuffer(&indexBufferView_);
	cList->SetGraphicsRootConstantBufferView(0, material_.GetMaterialResource()->GetGPUVirtualAddress());
	cList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
	cList->SetGraphicsRootDescriptorTable(2, material_.GetTexture()->gpuHandle);
	cList->DrawIndexedInstanced(6, 1, 0, 0, 0);

	ModelManager::GetInstance()->NormalCommand();
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

	float left = 0.0f - anchorPoint_.x;
	float right = 1.0f - anchorPoint_.x;
	float top = 0.0f - anchorPoint_.y;
	float bottom = 1.0f - anchorPoint_.y;

	if (isFlipX_) {
		left = -left;
		right = -right;
	}
	if (isFlipY_) {
		top = -top;
		bottom = -bottom;
	}

	vertex_[0].position = { left,top,0.0f,1.0f };
	vertex_[1].position = { left,bottom,0.0f,1.0f };
	vertex_[2].position = { right,bottom,0.0f,1.0f };
	vertex_[3].position = { right,top,0.0f,1.0f };
	std::memcpy(vData, vertex_.data(), sizeof(VertexDate) * vertex_.size());
}

void Sprite::SetRange(const Vector2& leftTop, const Vector2& size) {
	const DirectX::TexMetadata& meta = TextureManager::GetInstance()->GetMetaData(nowtexture);

	float left = leftTop.x / meta.width;
	float right = (leftTop.x + size.x) / meta.width;
	float top = leftTop.y / meta.height;
	float bottom = (leftTop.y + size.y) / meta.height;

	vertex_[0].texcoord = { left,top };
	vertex_[1].texcoord = { left,bottom };
	vertex_[2].texcoord = { right,bottom };
	vertex_[3].texcoord = { right,top };
	std::memcpy(vData, vertex_.data(), sizeof(VertexDate) * vertex_.size());
}

void Sprite::InitializeBuffer() {
	vertex_.push_back({ {0.0f,0.0f,0.0f,1.0f},{0.0f,0.0f},{0.0f,0.0f,-1.0f} });
	vertex_.push_back({ {0.0f,1.0f,0.0f,1.0f},{0.0f,1.0f},{0.0f,0.0f,-1.0f} });
	vertex_.push_back({ {1.0f,1.0f,0.0f,1.0f},{1.0f,1.0f},{0.0f,0.0f,-1.0f} });
	vertex_.push_back({ {1.0f,0.0f,0.0f,1.0f},{1.0f,0.0f},{0.0f,0.0f,-1.0f} });

	index_.push_back(0);
	index_.push_back(3);
	index_.push_back(1);

	index_.push_back(1);
	index_.push_back(3);
	index_.push_back(2);


	vertexResource_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(VertexDate) * vertex_.size());
	indexResource_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(uint32_t) * index_.size());

	vData = nullptr;
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vData));
	std::memcpy(vData, vertex_.data(), sizeof(VertexDate) * vertex_.size());

	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(VertexDate) * vertex_.size());
	vertexBufferView_.StrideInBytes = static_cast<UINT>(sizeof(VertexDate));

	uint32_t* iData = nullptr;
	indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&iData));
	std::memcpy(iData, index_.data(), sizeof(uint32_t) * index_.size());

	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
	indexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * index_.size());


	material_.CreateMaterial();
	material_.SetLightEnable(LightMode::kLightNone);


	wvpResource_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(TransformationMatrix));
	wvpData_ = nullptr;
	wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
	SetWvp();

	cameraPosResource_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(DirectionalLight));
	cameraPosData_ = nullptr;
	cameraPosResource_->Map(0, nullptr, reinterpret_cast<void**>(&cameraPosData_));
	cameraPosData_->worldPosition = { 0.0f,0.0f,0.0f };

	objIDDataResource_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(ObjIDData));
	objIDData_ = nullptr;
	objIDDataResource_->Map(0, nullptr, reinterpret_cast<void**>(&objIDData_));
	objIDData_->objID = -1;
}

void Sprite::AdjustTextureSize() {
	const DirectX::TexMetadata& meta = TextureManager::GetInstance()->GetMetaData(nowtexture);

	size_ = { static_cast<float>(meta.width),static_cast<float>(meta.height) };
	defaultSize_ = size_;
	SetWvp();
}

void Sprite::SetWvp() {
	Matrix4x4 worldMatrix = Multiply(Multiply(MakeScaleMatrix({ size_.x * scale_.x,size_.y * scale_.y,1.0f }), MakeRotateZMatrix(rotate_)), MakeTranslateMatrix(position_));
	Matrix4x4 viewMatrix = MakeIdentity4x4();

	Matrix4x4 projectionMatrix = MakeOrthographicMatrix(0.0f, 0.0f, float(MyWin::kWindowWidth), float(MyWin::kWindowHeight), 0.0f, 100.0f);
	Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));

	wvpData_->World = worldMatrix;
	wvpData_->WVP = worldViewProjectionMatrix;
	wvpData_->WorldInverseTransPose = Transpose(Inverse(wvpData_->World));
}
