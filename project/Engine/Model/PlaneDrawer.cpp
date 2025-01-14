#include "PlaneDrawer.h"
#include "DXCom.h"
#include "Camera.h"

PlaneDrawer::PlaneDrawer() {

	Initialize();
}

PlaneDrawer::~PlaneDrawer() {
	Finalize();
}


PlaneDrawer* PlaneDrawer::GetInstance() {
	static PlaneDrawer instance;
	return &instance;
}

std::unique_ptr<PlaneDrawer::PlaneData> PlaneDrawer::CreateMesh(UINT vertexCount, UINT indexCount) {

	DXCom* dxCommon = DXCom::GetInstance();
	std::unique_ptr<PlaneData> mesh = std::make_unique<PlaneData>();

	UINT vertBufferSize = sizeof(VertexPosUV) * vertexCount;
	mesh->vertBuffer = dxCommon->CreateBufferResource(dxCommon->GetDevice(), vertBufferSize);

	mesh->vbView.BufferLocation = mesh->vertBuffer->GetGPUVirtualAddress();
	mesh->vbView.StrideInBytes = sizeof(VertexPosUV);
	mesh->vbView.SizeInBytes = vertBufferSize;

	D3D12_RANGE readRange = { 0,0 };
	mesh->vertBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mesh->vertMap));


	UINT indexBufferSize = sizeof(uint16_t) * indexCount;
	if (indexCount > 0) {
		mesh->indexBuffer = dxCommon->CreateBufferResource(dxCommon->GetDevice(), indexBufferSize);

		mesh->ibView.BufferLocation = mesh->indexBuffer->GetGPUVirtualAddress();
		mesh->ibView.Format = DXGI_FORMAT_R16_UINT;
		mesh->ibView.SizeInBytes = indexBufferSize;

		mesh->indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mesh->indexMap));
	}

	return mesh;
}

void PlaneDrawer::Initialize() {

	CreateMeshes();
	CreateResource();

}

void PlaneDrawer::Finalize() {
}

void PlaneDrawer::AddPlanePoint(const Vector3& p1) {

	points_.push_back({ p1,{0.0f,0.0f} });

}

void PlaneDrawer::PopPlanePoint() {
	if (points_.empty()) {
		return;
	}
	points_.pop_front();
}

void PlaneDrawer::Reset() {

	points_.clear();

}

void PlaneDrawer::Render() {

	if (points_.size() < 4 ) {
		return;
	}


	ID3D12GraphicsCommandList* cList = DXCom::GetInstance()->GetCommandList();

	// カメラのビュープロジェクション行列を設定
	cBufferData_->viewProject = camera_->GetViewProjectionMatrix();

	DXCom::GetInstance()->GetPipelineManager()->SetPipeline(Pipe::Plane);
	cList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ポイントをvertMapに転送
	size_t planeCount = (points_.size()) / 2 - 1; // 平面の数
	size_t vertexIndex = 0;
	size_t indexIndex = 0;

	for (size_t i = 0; i < planeCount; ++i) {
		// 4つの頂点を取得
		for (size_t j = 0; j < 4; ++j) {
			planes_->vertMap[vertexIndex++] = points_[i * 2 + j];
		}

		// インデックスを設定
		planes_->indexMap[indexIndex++] = static_cast<uint16_t>(i * 4 + 0);
		planes_->indexMap[indexIndex++] = static_cast<uint16_t>(i * 4 + 1);
		planes_->indexMap[indexIndex++] = static_cast<uint16_t>(i * 4 + 2);

		planes_->indexMap[indexIndex++] = static_cast<uint16_t>(i * 4 + 1);
		planes_->indexMap[indexIndex++] = static_cast<uint16_t>(i * 4 + 2);
		planes_->indexMap[indexIndex++] = static_cast<uint16_t>(i * 4 + 3);
	}

	// 頂点とインデックスのビューを設定
	D3D12_VERTEX_BUFFER_VIEW vbView = planes_->vbView;
	vbView.SizeInBytes = static_cast<UINT>(sizeof(VertexPosUV) * vertexIndex);

	D3D12_INDEX_BUFFER_VIEW ibView = planes_->ibView;
	ibView.SizeInBytes = static_cast<UINT>(sizeof(uint16_t) * indexIndex);

	cList->IASetVertexBuffers(0, 1, &vbView);
	cList->IASetIndexBuffer(&ibView);

	// 定数バッファを設定
	cList->SetGraphicsRootConstantBufferView(0, cBufferResource_->GetGPUVirtualAddress());

	// 描画
	cList->DrawIndexedInstanced(static_cast<UINT>(indexIndex), 1, 0, 0, 0);
}


void PlaneDrawer::SetCamera(Camera* camera) {
	this->camera_ = camera;
}


void PlaneDrawer::CreateMeshes() {

	const UINT maxVertex = kMaxPlanes * 4;
	const UINT maxIndices = maxVertex * 120;

	planes_ = CreateMesh(maxVertex, maxIndices);
}

void PlaneDrawer::CreateResource() {

	DXCom* dxCommon = DXCom::GetInstance();

	cBufferResource_ = dxCommon->CreateBufferResource(dxCommon->GetDevice(), sizeof(CBuffer));
	cBufferData_ = nullptr;
	cBufferResource_->Map(0, nullptr, reinterpret_cast<void**>(&cBufferData_));
	cBufferData_->viewProject = MakeIdentity4x4();

}