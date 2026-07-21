#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include "../IGPUParticleEmitter.h"
#include "Engine/Math/Matrix/MatrixCalculation.h"
#include "Engine/Model/TextureManager.h"
#include "Engine/DX/FrameCount.h"

using Microsoft::WRL::ComPtr;


namespace Graphics {

	/// <summary>
	/// GPUパーティクルTextureエミッター
	/// </summary>
	struct EmitterTexture {
		Math::Vector3 translate;
		float radius;
		uint32_t count;
		float lifeTime;
		float frequency;
		float frequencyTime;
		uint32_t emit;

		// color
		Math::Vector3 colorMax;
		//float padding;
		Math::Vector3 colorMin;
		float padding2;

		// velocity
		Math::Vector3 baseVelocity;
		float velocityRandMax;
		float velocityRandMin;

	};

	/// <summary>
	/// テクスチャの絵柄をもとにパーティクルを発生させるエミッター
	/// </summary>
	class TextureBasedEmitter : public IGPUEmitter {
	public:

		TextureBasedEmitter(DXC::DXCom* dx);
		void InitTextureData(const std::string& fileName);

		void Update(float deltaTime) override;
		void Dispatch(ID3D12GraphicsCommandList* cmd,
			DXC::DXCom* dx, DXC::SRVManager* srv, const ParticleCSHandles& shared) override;
		void DebugGUI() override;
		void Save(const std::string& fileName) override;
		void Load(const std::string& fileName) override;

		// 一度だけエミット
		void Emit() override;
		bool IsEmit() const override { return isEmit_; }

		EmitterTexture& GetData() { return data_; }

		//========================================================================*/
		//* Setter
		void SetPos(const Math::Vector3& pos) override;
		void SetEmit(bool state) override { isEmit_ = state; }
		void SetCount(int count) override;
		void SetLifeTime(float lifeTime) override;
		void SetScale(const Math::Vector3& scale) override;
		void SetRadius(float radius) override;
		void SetVelocity(const Math::Vector3& vel) override;
		void SetColorRandom(const Math::Vector3& max, const Math::Vector3& min = Math::Vector3(0.0f, 0.0f, 0.0f)) override;
	private:
		void CopyData(uint32_t frameIndex = 0);

		EmitterTexture data_;
		bool isEmit_ = false;

		char saveName_[64] = "default";
		EmitterTexture* dataGPU_[DXC::kFrameCount_];
		Microsoft::WRL::ComPtr<ID3D12Resource> resource_[DXC::kFrameCount_];
		Texture* textureForEmit_;
		bool isOnceEmit_ = false;
	};

}
