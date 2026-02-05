#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include "../IGPUParticleEmitter.h"
#include "Math/Matrix/MatrixCalculation.h"
#include "Engine/DX/FrameCount.h"

using Microsoft::WRL::ComPtr;


/// <summary>
/// GPUパーティクル表面エミッター
/// </summary>
struct EmitterSurface {
	Math::Vector3 translate;
	float padding;
	Math::Vector3 scale = { 0.1f, 0.1f, 0.1f };
	float radius = 2.5f;
	uint32_t count = 500;
	float lifeTime = 1.0f;
	float frequency = 0.008f;
	float frequencyTime;
	uint32_t emit;

	// color
	Math::Vector3 colorMax = { 1.0f,1.0f,1.0f };
	Math::Vector3 colorMin = { 0.0f,0.0f,0.0f };
	float padding2;

	// velocity
	Math::Vector3 baseVelocity;
	float velocityRandMax;
	float velocityRandMin;

	int triangleCount;
};

class MeshSurfaceEmitter : public IGPUEmitter {
public:
	EmitterSurface data_;
	bool isEmit_ = false;

	MeshSurfaceEmitter(DXCom* dx);
	void InitMeshData(const std::string& fileName, DXCom* dx, SRVManager* srv);

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
	void SetCount(int count) override;
	void SetLifeTime(float lifeTime) override;
	void SetScale(const Math::Vector3& scale) override;
	void SetRadius(float radius) override;
	void SetColorRandom(const Math::Vector3& max, const Math::Vector3& min = Math::Vector3(0.0f, 0.0f, 0.0f)) override;
private:
	void CopyData(uint32_t frameIndex = 0);

	char saveName_[64] = "default";
	EmitterSurface* dataGPU_[DXC::kFrameCount_];
	ComPtr<ID3D12Resource> resource_[DXC::kFrameCount_];
	ComPtr<ID3D12Resource> verticesResource;
	ComPtr<ID3D12Resource> indiciesResource;
	ComPtr<ID3D12Resource> areasResource;
	uint32_t verticesIndex;
	uint32_t indiciesIndex;
	uint32_t areasIndex;
	bool isOnceEmit_ = false;
};