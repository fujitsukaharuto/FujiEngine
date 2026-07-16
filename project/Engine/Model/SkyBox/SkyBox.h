#pragma once
#include "ImGuiManager/ImGuiManager.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/SRVManager.h"
#include "Engine/Model/Model.h"
#include "Engine/Camera/CameraManager.h"
#include "Engine/Math/Vector/Vector4.h"
#include "Engine/DX/FrameCount.h"


namespace Graphics {
	/// <summary>
	/// SkyBox用クラス
	/// </summary>
	class SkyBox {
	public:
		SkyBox();
		~SkyBox();

		void Initialize();
		void Draw();
		void Render();
		void DebugGUI();

		/// <summary>行列更新</summary>
		void UpdateWVP();
		/// <summary>必要なクラスのSetter</summary>
		void SetCommonResources(DXCom* dxcommon, SRVManager* srvManager, Camera* camera);
		void SetColor(const Math::Vector4& color);

	private:

		void ResourceCreate();
		void CreateVertex();

	private:

		DXCom* dxcommon_;
		SRVManager* srvManager_;
		Camera* camera_;

		Math::Trans transform_;

		ComPtr<ID3D12Resource> vBuffer_;
		ComPtr<ID3D12Resource> iBuffer_;
		D3D12_VERTEX_BUFFER_VIEW vbView{};
		D3D12_INDEX_BUFFER_VIEW ibView{};

		std::vector<Graphics::VertexData> vertex_;
		std::vector<uint32_t> index_;

		Graphics::Material material_;

		Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_[DXC::kFrameCount_];
		Math::TransformationMatrix* wvpDataGPU_[DXC::kFrameCount_];

	};
}