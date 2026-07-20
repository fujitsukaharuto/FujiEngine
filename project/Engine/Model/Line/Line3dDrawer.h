#pragma once
#include <wrl/client.h>
#include "Math/Matrix/MatrixCalculation.h"
#include <cstdint>
#include <d3d12.h>
#include <memory>
#include "Engine/DX/FrameCount.h"

using namespace Microsoft::WRL;


namespace DXC { class DXCom; }

namespace Graphics {

	class Camera;

	/// <summary>
	/// 線描画管理クラス
	/// </summary>
	class Line3dDrawer {
	public:
		Line3dDrawer();
		~Line3dDrawer();

	public:

		static const UINT kMaxLineCount = 4096;
		static const UINT kVertexCountLine = 2;
		static const UINT kIndexCountLine = 0;

		/// <summary>
		/// 線を構成するためのデータ
		/// </summary>
		struct VertexPosColor {
			Math::Vector4 pos;
			Math::Vector4 color;
		};

		/// <summary>
		/// 線のデータ
		/// </summary>
		struct LineData {
			ComPtr<ID3D12Resource> vertBuffer[DXC::kFrameCount_];
			ComPtr<ID3D12Resource> indexBuffer;
			D3D12_VERTEX_BUFFER_VIEW vbView{};
			D3D12_INDEX_BUFFER_VIEW ibView{};
			VertexPosColor* vertMap[DXC::kFrameCount_];
			uint16_t* indexMap = nullptr;
		};


		static Line3dDrawer* GetInstance();

		std::unique_ptr<LineData> CreateMesh(UINT vertexCount, UINT indexCount);

		void Initialize(DXC::DXCom* pDxcom);

		void Finalize();

		/// <summary>
		/// 線の描画
		/// </summary>
		/// <param name="p1">点1</param>
		/// <param name="p2">点2</param>
		/// <param name="color">色</param>
		void DrawLine3d(const Math::Vector3& p1, const Math::Vector3& p2, const Math::Vector4& color);

		/// <summary>球を線で描画する</summary>
		void DrawSphereLine(const Math::Vector3& center, float radius, const Math::Vector4& color);

		/// <summary>ボーンを線で描画する</summary>
		void DrawOctahedralBone(const Math::Vector3& base, const Math::Vector3& tip);

		void Reset();

		void Render();

		void SetCamera(Camera* camera);


	private:

		void CreateMeshes();

		void CreateResource();

	private:

		std::unique_ptr<LineData> line_;

		uint32_t indexLine_ = 0;

		DXC::DXCom* dxcommon_;
		Camera* camera_ = nullptr;

		struct CBuffer {
			Math::Matrix4x4 viewProject;
		};

		ComPtr<ID3D12Resource> cBufferResource_[DXC::kFrameCount_];
		CBuffer* cBufferData_[DXC::kFrameCount_];

	};
}