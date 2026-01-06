#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include "../IGPUParticleEmitter.h"
#include "Math/Matrix/MatrixCalculation.h"
#include "Engine/DX/FrameCount.h"

using Microsoft::WRL::ComPtr;


/// <summary>
/// GPUパーティクルエミッター
/// </summary>
struct EmitterSphere {
	Math::Vector3 translate;
	float radius;

	Math::Vector3 scale;
	uint32_t count;

	Math::Vector3 colorMax;
	float  lifeTime;

	Math::Vector3 colorMin;
	float frequency;

	Math::Vector3 baseVelocity;
	float velocityRandMax;

	float velocityRandMin;
	uint32_t emit;
	uint32_t isDistance;
	float frequencyTime;

	Math::Vector3 prevTranslate;
	uint32_t emitShapeType;

	Math::Quaternion rotation;

	uint32_t emitVeloType;
	uint32_t isRandomMove;
	uint32_t isTrailEmit;
	uint32_t isGravity;

	/*uint32_t isColorFadeIn;
	uint32_t isAddRandomSize;
	uint32_t isTwinkle;*/
};

class SphereEmitter : public IGPUEmitter {
public:
	EmitterSphere data_;
	bool isEmit_ = false;

	SphereEmitter(DXCom* dx);

	void Update(float deltaTime) override;
	void Dispatch(ID3D12GraphicsCommandList* cmd,
		DXCom* dx, SRVManager* srv, const ParticleCSHandles& shared) override;
	void DebugGUI() override;
	void Save(const std::string& fileName) override;
	void Load(const std::string& fileName) override;

	void Emit() override;
	bool IsEmit() const override { return isEmit_; }
	void SetPos(const Math::Vector3& pos) override;
	void SetEmit(bool state) override;
private:
	void CopyData(uint32_t frameIndex = 0);

	char saveName_[64] = "default";
	EmitterSphere* dataGPU_[DXC::kFrameCount_];
	ComPtr<ID3D12Resource> resource_[DXC::kFrameCount_];
	bool isOnceEmit_ = false;
};