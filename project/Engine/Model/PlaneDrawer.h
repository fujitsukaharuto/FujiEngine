#pragma once
#include <wrl/client.h>
#include "MatrixCalculation.h"
#include <cstdint>
#include <d3d12.h>
#include <memory>
#include <string>
#include <deque>

using namespace Microsoft::WRL;


class Camera;

class PlaneDrawer {
public:
	PlaneDrawer();
	~PlaneDrawer();

public:

	static const size_t kMaxPlanes = 30;

	struct VertexPosUV {
		Vector3 pos;
		Vector2 uv;
		Vector4 color;
	};

	struct PlaneData {
		ComPtr<ID3D12Resource> vertBuffer;
		ComPtr<ID3D12Resource> indexBuffer;
		D3D12_VERTEX_BUFFER_VIEW vbView{};
		D3D12_INDEX_BUFFER_VIEW ibView{};
		VertexPosUV* vertMap = nullptr;
		uint16_t* indexMap = nullptr;
	};


	static PlaneDrawer* GetInstance();

	std::unique_ptr<PlaneData> CreateMesh(UINT vertexCount, UINT indexCount);

	void Initialize();

	void Finalize();

	void AddPlanePoint(const Vector3& p1);

	void PopPlanePoint();

	void Reset();

	void Render();

	void SetCamera(Camera* camera);


private:

	void CreateMeshes();
	void CreateResource();

private:

	std::unique_ptr<PlaneData> planes_;
	std::deque<VertexPosUV> points_;

	int numberCount_ = 0;

	uint32_t indexPlane_ = 0;

	Camera* camera_ = nullptr;
	struct CBuffer {
		Matrix4x4 viewProject;
	};
	ComPtr<ID3D12Resource> cBufferResource_ = nullptr;
	CBuffer* cBufferData_ = nullptr;

};