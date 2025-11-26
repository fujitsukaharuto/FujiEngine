#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include "../IGPUParticleEmitter.h"
#include "Math/Matrix/MatrixCalculation.h"
#include "Engine/Model/TextureManager.h"
#include "Engine/Editor/JsonSerializer.h"

using Microsoft::WRL::ComPtr;


/// <summary>
/// GPUパーティクルTextureエミッター
/// </summary>
struct EmitterTexture {
	Vector3 translate;
	float radius;
	uint32_t count;
	float lifeTime;
	float frequency;
	float frequencyTime;
	uint32_t emit;

	// color
	Vector3 colorMax;
	//float padding;
	Vector3 colorMin;
	float padding2;

	// velocity
	Vector3 baseVelocity;
	float velocityRandMax;
	float velocityRandMin;

};

class TextureBasedEmitter : public IGPUEmitter {
public:
	EmitterTexture* data_ = nullptr;
	ComPtr<ID3D12Resource> resource_;
	Texture* textureForEmit_;
	bool isEmit_ = false;

	TextureBasedEmitter(DXCom* dx);
	void InitTextureData(const std::string& fileName);

	void Update(float deltaTime) override;
	void Dispatch(ID3D12GraphicsCommandList* cmd,
		DXCom* dx, SRVManager* srv, const ParticleCSHandles& shared) override;
	void DebugGUI() override;
	void Save(const std::string& fileName) override;
	void Load(const std::string& fileName) override;

	bool IsEmit() const override { return isEmit_; }
	void SetEmit(bool state) override { isEmit_ = state; }
private:
	char saveName_[64] = "default";
};