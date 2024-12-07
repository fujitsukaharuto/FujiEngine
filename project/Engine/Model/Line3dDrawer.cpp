#include "Line3dDrawer.h"
#include "DXCom.h"
#include "Camera.h"

Line3dDrawer::Line3dDrawer() {

	Initialize();
}

Line3dDrawer::~Line3dDrawer() {}

Line3dDrawer* Line3dDrawer::GetInstance() {
	static Line3dDrawer instance;
	return &instance;
}

std::unique_ptr<Line3dDrawer::LineData> Line3dDrawer::CreateMesh(UINT vertexCount, UINT indexCount) {

	DXCom* dxCommon = DXCom::GetInstance();
	std::unique_ptr<LineData> mesh = std::make_unique<LineData>();

	UINT vertBufferSize = sizeof(VertexPosColor) * vertexCount;
	mesh->vertBuffer = dxCommon->CreateBufferResource(dxCommon->GetDevice(), vertBufferSize);

	mesh->vbView.BufferLocation = mesh->vertBuffer->GetGPUVirtualAddress();
	mesh->vbView.StrideInBytes = sizeof(VertexPosColor);
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

void Line3dDrawer::Initialize() {

	CreateMeshes();
	CreateResource();

}

void Line3dDrawer::Finalize() {
}

void Line3dDrawer::DrawLine3d(const Vector3& p1, const Vector3& p2, const Vector4& color) {

	if (indexLine_ < kMaxLineCount) {
		line_->vertMap[indexLine_ * 2] = { p1,color };
		line_->vertMap[indexLine_ * 2 + 1] = { p2,color };

		indexLine_++;
	}
}

void Line3dDrawer::Reset() {
	indexLine_ = 0;
}

void Line3dDrawer::Render() {

	if (indexLine_ == 0) {
		return;
	}

	ID3D12GraphicsCommandList* cList = DXCom::GetInstance()->GetCommandList();

	cBufferData_->viewProject = camera_->GetViewProjectionMatrix();

	DXCom::GetInstance()->GetPipelineManager()->SetPipeline(Pipe::Line3d);
	cList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

	D3D12_VERTEX_BUFFER_VIEW vbView = line_->vbView;
	cList->IASetVertexBuffers(0, 1, &vbView);
	cList->SetGraphicsRootConstantBufferView(0, cBufferResource_->GetGPUVirtualAddress());
	cList->DrawInstanced(indexLine_ * 2, 1, 0, 0);

	Reset();

}

void Line3dDrawer::SetCamera(Camera* camera) {
	this->camera_ = camera;
}


void Line3dDrawer::CreateMeshes() {

	const UINT maxVertex = kMaxLineCount * kVertexCountLine;
	const UINT maxIndices = 20;

	line_ = CreateMesh(maxVertex, maxIndices);
}

void Line3dDrawer::CreateResource() {

	DXCom* dxCommon = DXCom::GetInstance();

	cBufferResource_ = dxCommon->CreateBufferResource(dxCommon->GetDevice(), sizeof(CBuffer));
	cBufferData_ = nullptr;
	cBufferResource_->Map(0, nullptr, reinterpret_cast<void**>(&cBufferData_));
	cBufferData_->viewProject = MakeIdentity4x4();

}