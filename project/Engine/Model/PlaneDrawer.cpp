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

void PlaneDrawer::AddPlanePoint(const Vector3& p1, const Vector3& p2) {




}

void PlaneDrawer::Reset() {

}

void PlaneDrawer::Render() {



}

void PlaneDrawer::SetCamera(Camera* camera) {
	this->camera_ = camera;
}


void PlaneDrawer::CreateMeshes() {


}

void PlaneDrawer::CreateResource() {

	DXCom* dxCommon = DXCom::GetInstance();

	cBufferResource_ = dxCommon->CreateBufferResource(dxCommon->GetDevice(), sizeof(CBuffer));
	cBufferData_ = nullptr;
	cBufferResource_->Map(0, nullptr, reinterpret_cast<void**>(&cBufferData_));
	cBufferData_->viewProject = MakeIdentity4x4();

}