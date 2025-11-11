#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include "../IGPUParticleEmitter.h"
#include "Math/Matrix/MatrixCalculation.h"

using Microsoft::WRL::ComPtr;


/// <summary>
/// GPUパーティクルエミッター
/// </summary>
struct EmitterSphere {
	Vector3 translate;
	float padding;
	Vector3 scale;
	float radius;
	uint32_t count;
	float lifeTime;
	float frequency;
	float frequencyTime;
	uint32_t emit;

	// color
	Vector3 colorMax;
	Vector3 colorMin;
	float padding2;

	// velocity
	Vector3 baseVelocity;
	float velocityRandMax;
	float velocityRandMin;

	// distanceEmit
	Vector3 prevTranslate;
	float padding4;

};

class SphereEmitter : public IGPUEmitter {
public:
	EmitterSphere* data_ = nullptr;
	ComPtr<ID3D12Resource> resource_;
	bool isEmit_ = false;

	SphereEmitter(DXCom* dx);

	void Update(float deltaTime) override;
	void Dispatch(ID3D12GraphicsCommandList* cmd,
		DXCom* dx, SRVManager* srv, const ParticleCSHandles& shared) override;
	void DebugGUI() override;

	bool IsEmit() const override { return isEmit_; }
	void SetEmit(bool state) override { isEmit_ = state; }
};