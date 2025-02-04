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

void PlaneDrawer::AddPlanePoint(const Vector3& p1, const Vector3& color) {

	if (numberCount_ == 0) {
		points_.push_back({ p1,{0.0f,0.0f},{color.x,color.y,color.z,0.2f} });
		numberCount_++;
	}
	else {
		points_.push_back({ p1,{0.0f,0.0f},{color.x,color.y,color.z,0.8f} });
		numberCount_ = 0;
	}

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

		float step = 1.0f / static_cast<float>(planeCount);

		for (size_t j = 0; j < 4; ++j) {

			planes_->vertMap[vertexIndex] = points_[i * 2 + j];

			// UV座標を設定
			if (j == 0) {
				planes_->vertMap[vertexIndex].uv = { step * i , 0.0f };   // 左上
			}
			else if (j == 1) {
				planes_->vertMap[vertexIndex].uv = { step * i , 1.0f };   // 左下
			}
			else if (j == 2) {
				planes_->vertMap[vertexIndex].uv = { step + (step * i) , 0.0f };   // 右上
			}
			else if (j == 3) {
				planes_->vertMap[vertexIndex].uv = { step + (step * i), 1.0f };   // 右下
			}
			vertexIndex++;
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

	int addT = 0;
	float alphaT = 2.0f;
	for (auto& point : points_) {
		point.color.w -= (0.1f * alphaT) * FPSKeeper::DeltaTime();
		if (point.color.w < 0.0f) {
			point.color.w = 0.0f;
		}
		addT++;
		if (addT == 4) {
			addT = 0;
			alphaT = -0.04f;
			if (alphaT < 0.1f) {
				alphaT = 0.1f;
			}
		}
	}

	//points_.erase(
	//	std::remove_if(points_.begin(), points_.end(),
	//		[](const VertexPosUV& point) {
	//			return point.color.w == 0.0f; // α値が0かチェック
	//		}),
	//	points_.end()
	//);

}


void PlaneDrawer::SetCamera(Camera* camera) {
	this->camera_ = camera;
}


void PlaneDrawer::CreateMeshes() {

	const UINT maxVertex = kMaxPlanes * 4;
	const UINT maxIndices = maxVertex * 400;

	planes_ = CreateMesh(maxVertex, maxIndices);
}

void PlaneDrawer::CreateResource() {

	DXCom* dxCommon = DXCom::GetInstance();

	cBufferResource_ = dxCommon->CreateBufferResource(dxCommon->GetDevice(), sizeof(CBuffer));
	cBufferData_ = nullptr;
	cBufferResource_->Map(0, nullptr, reinterpret_cast<void**>(&cBufferData_));
	cBufferData_->viewProject = MakeIdentity4x4();

}