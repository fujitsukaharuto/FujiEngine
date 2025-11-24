#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include "../IGPUParticleEmitter.h"
#include "Math/Matrix/MatrixCalculation.h"

using Microsoft::WRL::ComPtr;


/// <summary>
/// GPUパーティクル表面エミッター
/// </summary>
struct EmitterSurface {
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

	int triangleCount;
};

class MeshSurefaceEmitter : public IGPUEmitter {
public:
	EmitterSurface* data_ = nullptr;
	ComPtr<ID3D12Resource> resource_;
	ComPtr<ID3D12Resource> verticesResource;
	ComPtr<ID3D12Resource> indiciesResource;
	ComPtr<ID3D12Resource> areasResource;
	uint32_t verticesIndex;
	uint32_t indiciesIndex;
	uint32_t areasIndex;
	bool isEmit_ = false;

	MeshSurefaceEmitter(DXCom* dx);
	void InitMeshData(const std::string& fileName, DXCom* dx, SRVManager* srv);

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