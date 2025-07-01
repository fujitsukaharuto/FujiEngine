#include "Line3dDrawer.h"
#include "Engine/DX/DXCom.h"
#include "Camera.h"

Line3dDrawer::Line3dDrawer() {
}

Line3dDrawer::~Line3dDrawer() {}

Line3dDrawer* Line3dDrawer::GetInstance() {
	static Line3dDrawer instance;
	return &instance;
}

std::unique_ptr<Line3dDrawer::LineData> Line3dDrawer::CreateMesh(UINT vertexCount, UINT indexCount) {

	std::unique_ptr<LineData> mesh = std::make_unique<LineData>();

	UINT vertBufferSize = sizeof(VertexPosColor) * vertexCount;
	mesh->vertBuffer = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), vertBufferSize);

	mesh->vbView.BufferLocation = mesh->vertBuffer->GetGPUVirtualAddress();
	mesh->vbView.StrideInBytes = sizeof(VertexPosColor);
	mesh->vbView.SizeInBytes = vertBufferSize;

	D3D12_RANGE readRange = { 0,0 };
	mesh->vertBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mesh->vertMap));


	UINT indexBufferSize = sizeof(uint16_t) * indexCount;
	if (indexCount > 0) {
		mesh->indexBuffer = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), indexBufferSize);

		mesh->ibView.BufferLocation = mesh->indexBuffer->GetGPUVirtualAddress();
		mesh->ibView.Format = DXGI_FORMAT_R16_UINT;
		mesh->ibView.SizeInBytes = indexBufferSize;

		mesh->indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mesh->indexMap));
	}

	return mesh;
}

void Line3dDrawer::Initialize(DXCom* pDxcom) {

	dxcommon_ = pDxcom;

	CreateMeshes();
	CreateResource();

}

void Line3dDrawer::Finalize() {
	line_.reset();
	cBufferResource_.Reset();
	camera_ = nullptr;
	dxcommon_ = nullptr;
}

void Line3dDrawer::DrawLine3d(const Vector3& p1, const Vector3& p2, const Vector4& color) {

	if (indexLine_ < kMaxLineCount) {
		line_->vertMap[indexLine_ * 2] = { p1,color };
		line_->vertMap[indexLine_ * 2 + 1] = { p2,color };

		indexLine_++;
	}
}

void Line3dDrawer::DrawShereLine(const Vector3& center, float radius, const Vector4& color) {
	const int segments = 16;
	for (int i = 0; i < segments; i++) {
		float theta1 = ((float)i / (float)segments) * 2.0f * std::numbers::pi_v<float>;
		float theta2 = ((float)(i + 1) / (float)segments) * 2.0f * std::numbers::pi_v<float>;

		// XY平面円
		DrawLine3d(
			center + Vector3{ cosf(theta1), sinf(theta1), 0.0f } *radius,
			center + Vector3{ cosf(theta2), sinf(theta2), 0.0f } *radius,
			color
		);

		// XZ平面円
		DrawLine3d(
			center + Vector3{ cosf(theta1), 0.0f, sinf(theta1) } *radius,
			center + Vector3{ cosf(theta2), 0.0f, sinf(theta2) } *radius,
			color
		);

		// YZ平面円
		DrawLine3d(
			center + Vector3{ 0.0f, cosf(theta1), sinf(theta1) } *radius,
			center + Vector3{ 0.0f, cosf(theta2), sinf(theta2) } *radius,
			color
		);
	}
}

void Line3dDrawer::DrawOctahedralBone(const Vector3& base, const Vector3& tip) {
	Vector3 center = (base + tip) * 0.5f;
	Vector3 dir = tip - base;
	float length = dir.Length();
	Vector3 dirN = dir.Normalize();

	// 垂直軸（Blender同様の補助ベクトル）
	Vector3 up = Vector3::Up();
	if (fabs(Vector3::Dot(up, dirN)) > 0.95f) {
		up = Vector3::Right();
	}
	Vector3 side1 = Vector3::Cross(dirN, up).Normalize();
	Vector3 side2 = Vector3::Cross(dirN, side1).Normalize();

	float width = length * 0.1f;

	Vector3 left = center + side1 * width;
	Vector3 right = center - side1 * width;
	Vector3 top = center + side2 * width;
	Vector3 bottom = center - side2 * width;

	// base → 4 中心突起へ
	DrawLine3d(base, left, { 1,1,1,1 });
	DrawLine3d(base, right, { 1,1,1,1 });
	DrawLine3d(base, top, { 1,1,1,1 });
	DrawLine3d(base, bottom, { 1,1,1,1 });

	// tip → 4 中心突起へ
	DrawLine3d(tip, left, { 1,1,1,1 });
	DrawLine3d(tip, right, { 1,1,1,1 });
	DrawLine3d(tip, top, { 1,1,1,1 });
	DrawLine3d(tip, bottom, { 1,1,1,1 });

	// 中心突起を輪でつなぐ（オプション）
	DrawLine3d(left, right, { 1,1,1,1 });
	DrawLine3d(right, bottom, { 1,1,1,1 });
	DrawLine3d(bottom, left, { 1,1,1,1 });
	DrawLine3d(top, left, { 1,1,1,1 });
	DrawLine3d(top, right, { 1,1,1,1 });
	DrawLine3d(top, bottom, { 1,1,1,1 });
}

void Line3dDrawer::Reset() {
	indexLine_ = 0;
}

void Line3dDrawer::Render() {

	if (indexLine_ == 0) {
		return;
	}

	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();

	cBufferData_->viewProject = camera_->GetViewProjectionMatrix();

	dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Line3d);
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

	cBufferResource_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(CBuffer));
	cBufferData_ = nullptr;
	cBufferResource_->Map(0, nullptr, reinterpret_cast<void**>(&cBufferData_));
	cBufferData_->viewProject = MakeIdentity4x4();

}