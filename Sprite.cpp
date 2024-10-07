#include "Sprite.h"
#include "DXCom.h"

void Sprite::Load(const std::string& fileName) {

	material_.SetTextureNamePath(fileName);
	InitializeBuffer();

}

void Sprite::Draw() {
	ID3D12GraphicsCommandList* cList = DXCom::GetInstance()->GetCommandList();


	cList->IASetVertexBuffers(0, 1, &vertexBufferView_);
	cList->IASetIndexBuffer(&indexBufferView_);
	cList->SetGraphicsRootConstantBufferView(0, material_.GetMaterialResource()->GetGPUVirtualAddress());
	cList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
	cList->SetGraphicsRootConstantBufferView(3, directionalLightResource_->GetGPUVirtualAddress());
	cList->SetGraphicsRootDescriptorTable(2, material_.GetTexture()->gpuHandle);
	cList->DrawIndexedInstanced(6, 1, 0, 0, 0);

}

void Sprite::SetPos(const Vector3& pos) {
	position_ = pos;
}

void Sprite::SetSize(const Vector2& size) {
	size_ = size;
}

void Sprite::SetAngle(float rotate) {
	rotate_ = rotate;
}

void Sprite::InitializeBuffer() {

	vertex_.push_back({ {-1.0f,-1.0f,0.0f,1.0f},{0.0f,0.0f},{0.0f,0.0f,-1.0f} });
	vertex_.push_back({ {-1.0f,1.0f,0.0f,1.0f},{0.0f,1.0f},{0.0f,0.0f,-1.0f} });
	vertex_.push_back({ {1.0f,1.0f,0.0f,1.0f},{1.0f,1.0f},{0.0f,0.0f,-1.0f} });
	vertex_.push_back({ {1.0f,-1.0f,0.0f,1.0f},{1.0f,0.0f},{0.0f,0.0f,-1.0f} });

	index_.push_back(0);
	index_.push_back(3);
	index_.push_back(1);

	index_.push_back(1);
	index_.push_back(3);
	index_.push_back(2);


	vertexResource_ = DXCom::GetInstance()->CreateBufferResource(DXCom::GetInstance()->GetDevice(), sizeof(VertexDate) * vertex_.size());
	indexResource_ = DXCom::GetInstance()->CreateBufferResource(DXCom::GetInstance()->GetDevice(), sizeof(uint32_t) * index_.size());
	
	VertexDate* vData = nullptr;
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


	wvpResource_ = DXCom::GetInstance()->CreateBufferResource(DXCom::GetInstance()->GetDevice(), sizeof(TransformationMatrix));
	wvpData_ = nullptr;
	wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
	SetWvp();

	directionalLightResource_ = DXCom::GetInstance()->CreateBufferResource(DXCom::GetInstance()->GetDevice(), sizeof(DirectionalLight));
	directionalLightData_ = nullptr;
	directionalLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData_));
	directionalLightData_->color = { 1.0f,1.0f,1.0f,1.0f };
	directionalLightData_->direction = { 1.0f,0.0f,0.0f };
	directionalLightData_->intensity = 1.0f;

}

void Sprite::SetWvp() {
	Matrix4x4 worldMatrix = Multiply(Multiply(MakeScaleMatrix({ size_.x,size_.y,1.0f }), MakeRotateZMatrix(rotate_)), MakeTranslateMatrix(position_));
	Matrix4x4 viewMatrix = DXCom::GetInstance()->GetView();

	Matrix4x4 projectionMatrix = MakeOrthographicMatrix(0.0f, 0.0f, float(MyWin::kWindowWidth), float(MyWin::kWindowHeight), 0.0f, 100.0f);
	Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));

	wvpData_->World = worldMatrix;
	wvpData_->WVP = worldViewProjectionMatrix;

}
