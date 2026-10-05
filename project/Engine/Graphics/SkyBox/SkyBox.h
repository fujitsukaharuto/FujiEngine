#pragma once
#include <wrl/client.h>
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/SRVManager.h"
#include "Engine/Graphics/Model/Model.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Math/Vector/Vector4.h"
#include "Engine/DXC/FrameCount.h"


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
		void SetColor(const Math::Vector4& color);

		/// <summary>画面に出る空の、地平線あたりの色。フォグの色を空に合わせるのに使う</summary>
		/// <remarks>テクスチャから測った値に、今の色(SetColor)と Skybox.PS と同じ露光・トーンマップを掛けたもの</remarks>
		/// <param name="side">0:+X 1:-X 2:+Z 3:-Z</param>
		Math::Vector3 GetHorizonColor(int side);

	private:

		void ResourceCreate();
		void CreateVertex();
		/// <summary>テクスチャの横4面から地平線の色を測る。ファイルを読み直すので最初の1回だけ</summary>
		void MeasureHorizonColor();

	private:

		DXC::DXCom* dxcommon_;
		DXC::SRVManager* srvManager_;
		Camera* camera_;

		Math::Trans transform_;

		Microsoft::WRL::ComPtr<ID3D12Resource> vBuffer_;
		Microsoft::WRL::ComPtr<ID3D12Resource> iBuffer_;
		D3D12_VERTEX_BUFFER_VIEW vbView{};
		D3D12_INDEX_BUFFER_VIEW ibView{};

		std::vector<Graphics::VertexData> vertex_;
		std::vector<uint32_t> index_;

		Graphics::Material material_;
		// テクスチャそのままの値(色も露光も掛ける前)。+X, -X, +Z, -Z の順
		Math::Vector3 horizonColor_[4]{};

		Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_[DXC::kFrameCount_];
		Math::TransformationMatrix* wvpDataGPU_[DXC::kFrameCount_];

	};
}