#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include "../IGPUParticleEmitter.h"
#include "Math/Matrix/MatrixCalculation.h"
#include "Engine/Model/TextureManager.h"
#include "Engine/Editor/JsonSerializer.h"
#include "Engine/DX/FrameCount.h"

using Microsoft::WRL::ComPtr;


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

class TextureBasedEmitter : public IGPUEmitter {
public:
	EmitterTexture data_;
	bool isEmit_ = false;

	TextureBasedEmitter(DXCom* dx);
	void InitTextureData(const std::string& fileName);

	void Update(float deltaTime) override;
	void Dispatch(ID3D12GraphicsCommandList* cmd,
		DXCom* dx, SRVManager* srv, const ParticleCSHandles& shared) override;
	void DebugGUI() override;
	void Save(const std::string& fileName) override;
	void Load(const std::string& fileName) override;

	void Emit() override;
	bool IsEmit() const override { return isEmit_; }
	void SetPos(const Math::Vector3& pos) override;
	void SetEmit(bool state) override { isEmit_ = state; }
private:
	void CopyData(uint32_t frameIndex = 0);

	char saveName_[64] = "default";
	EmitterTexture* dataGPU_[DXC::kFrameCount_];
	ComPtr<ID3D12Resource> resource_[DXC::kFrameCount_];
	Texture* textureForEmit_;
	bool isOnceEmit_ = false;
};