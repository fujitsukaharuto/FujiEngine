#pragma once
#include <wrl/client.h>
#include "Engine/Math/Matrix/MatrixCalculation.h"
#include "Engine/Graphics/Model/Material/Material.h"
#include "Engine/Graphics/Object/Object3d.h"
#include "Engine/DXC/FrameCount.h"

namespace DXC { class DXCom; }
class PointLight;
class SpotLight;

namespace Graphics {
	/// <summary>
	/// スプライト描画クラス（データ保持用）
	/// </summary>
	class Sprite {
	public:
		Sprite();
		~Sprite();

	public:
		void Load(const std::string& fileName);
		void Draw();

		//========================================================================*/
		//* Setter
		void SetColor(const Math::Vector4& color);
		void SetPos(const Math::Vector3& pos);
		void SetScale(const Math::Vector2& scale);
		void SetSize(const Math::Vector2& size);
		void SetAngle(float rotate);
		void SetAnchor(const Math::Vector2& anchor);

		void SetFlipX(bool is) { isFlipX_ = is; SetWvp(); }
		void SetFlipY(bool is) { isFlipY_ = is; SetWvp(); }

		void SetRange(const Math::Vector2& leftTop, const Math::Vector2& size);

		//========================================================================*/
		//* Getter
		Math::Vector2 GetDefaultSize() const { return defaultSize_; }

		// ↓ レンダラーが描画に使うためのデータを渡すGetterを追加
		D3D12_GPU_VIRTUAL_ADDRESS GetWvpGPUAddress(uint32_t frameIndex) const;
		D3D12_GPU_VIRTUAL_ADDRESS GetMaterialGPUAddress() ;
		D3D12_GPU_DESCRIPTOR_HANDLE GetTextureSRV() ;

	private:
		void AdjustTextureSize();
		void SetWvp();

	private:
		DXC::DXCom* dxcommon_;

		Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_[DXC::kFrameCount_];
		Math::TransformationMatrix* wvpDataGPU_[DXC::kFrameCount_];

		Microsoft::WRL::ComPtr<ID3D12Resource> cameraPosResource_[DXC::kFrameCount_];
		Math::CameraForGPU* cameraPosData_[DXC::kFrameCount_];

		Microsoft::WRL::ComPtr<ID3D12Resource> objIDDataResource_ = nullptr;
		ObjIDData* objIDData_ = nullptr;

		Graphics::Material material_;
		std::string nowTexture;

		Math::Vector2 anchorPoint_{ 0.5f,0.5f };
		Math::Vector3 position_ = { 0,0,0 };
		Math::Vector2 defaultSize_ = {};
		Math::Vector2 size_ = { 200, 200 };
		Math::Vector2 scale_ = { 1.0f,1.0f };
		float rotate_ = 0.0f;

		bool isFlipX_ = false;
		bool isFlipY_ = false;
	};
}