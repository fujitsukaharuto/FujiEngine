#include "ParticleManager.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/SRVManager.h"
#include "Engine/Camera/CameraManager.h"
#include "Particle.h"
#include "Math/Random/Random.h"
#include "Engine/DX/FPSKeeper.h"
#include "Engine/Model/ModelManager.h"
#include "ImGuiManager/ImGuiManager.h"
#include "Engine/Editor/JsonSerializer.h"
#include "Engine/DX/FrameCount.h"
#include <fstream>
#include <filesystem>

using namespace Core;
using namespace Graphics;
using namespace Math;



void ParticleManager::Initialize(DXCom* pDxcom, SRVManager* srvManager) {
	dxcommon_ = pDxcom;
	srvManager_ = srvManager;
	this->camera_ = CameraManager::GetInstance()->GetCamera();

	InitPlaneVertex();
	InitRingVertex();
	InitSphereVertex();
	InitCylinderVertex();
	InitLightningVertex();

	gpuParticleSystem_ = std::make_unique<GPUParticleSystem>();
	gpuParticleSystem_->Initialize(pDxcom, srvManager);

	LoadCSEmitterFileDir();
}

int ParticleManager::InitGPUEmitter(int returnMod) {
	return gpuParticleSystem_->InitGPUEmitter(returnMod);
}

int ParticleManager::InitGPUEmitterTexture(const std::string& fileName) {
	return gpuParticleSystem_->InitGPUEmitterTexture(fileName);
}

int ParticleManager::InitGPUEmitterSurface(const std::string& fileName) {
	return gpuParticleSystem_->InitGPUEmitterSurface(fileName);
}

void ParticleManager::InitDefaultCSEmitter() {
	gpuParticleSystem_->InitDefaultEmitter();
}

void ParticleManager::InitPlaneVertex() {
	plane_.vertices.push_back({ {-1.0f,1.0f,0.0f,1.0f},{0.0f,0.0f},{0.0f,0.0f,-1.0f} });
	plane_.vertices.push_back({ {-1.0f,-1.0f,0.0f,1.0f},{0.0f,1.0f},{0.0f,0.0f,-1.0f} });
	plane_.vertices.push_back({ {1.0f,-1.0f,0.0f,1.0f},{1.0f,1.0f},{0.0f,0.0f,-1.0f} });
	plane_.vertices.push_back({ {1.0f,1.0f,0.0f,1.0f},{1.0f,0.0f},{0.0f,0.0f,-1.0f} });

	plane_.indices.push_back(0);
	plane_.indices.push_back(3);
	plane_.indices.push_back(1);

	plane_.indices.push_back(1);
	plane_.indices.push_back(3);
	plane_.indices.push_back(2);

	plane_.vBuffer = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(VertexDate) * plane_.vertices.size());
	plane_.iBuffer = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(uint32_t) * plane_.indices.size());

	VertexDate* vData = nullptr;
	plane_.vBuffer->Map(0, nullptr, reinterpret_cast<void**>(&vData));
	std::memcpy(vData, plane_.vertices.data(), sizeof(VertexDate) * plane_.vertices.size());

	plane_.vbView.BufferLocation = plane_.vBuffer->GetGPUVirtualAddress();
	plane_.vbView.SizeInBytes = static_cast<UINT>(sizeof(VertexDate) * plane_.vertices.size());
	plane_.vbView.StrideInBytes = static_cast<UINT>(sizeof(VertexDate));

	uint32_t* iData = nullptr;
	plane_.iBuffer->Map(0, nullptr, reinterpret_cast<void**>(&iData));
	std::memcpy(iData, plane_.indices.data(), sizeof(uint32_t) * plane_.indices.size());

	plane_.ibView.BufferLocation = plane_.iBuffer->GetGPUVirtualAddress();
	plane_.ibView.Format = DXGI_FORMAT_R32_UINT;
	plane_.ibView.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * plane_.indices.size());
}

void ParticleManager::InitRingVertex() {
	const uint32_t kRingDivide = 32;
	const float kOuterRadius = 1.0f;
	const float kInnerRadius = 0.2f;
	const float radianPerDivide = 2.0f * std::numbers::pi_v<float> / float(kRingDivide);

	for (uint32_t i = 0; i <= kRingDivide; i++) {
		float angle = i * radianPerDivide;
		float sinA = std::sin(angle);
		float cosA = std::cos(angle);
		float u = float(i) / float(kRingDivide);

		// 外周
		ring_.vertices.push_back({ {-sinA * kOuterRadius, cosA * kOuterRadius, 0.0f, 1.0f}, {u, 0.0f}, {0,0,1} });
		// 内周
		ring_.vertices.push_back({ {-sinA * kInnerRadius, cosA * kInnerRadius, 0.0f, 1.0f}, {u, 1.0f}, {0,0,1} });
	}

	// インデックス生成
	for (uint32_t i = 0; i < kRingDivide; i++) {
		uint32_t outer0 = i * 2;
		uint32_t inner0 = outer0 + 1;
		uint32_t outer1 = outer0 + 2;
		uint32_t inner1 = outer0 + 3;

		// 三角形1
		ring_.indices.push_back(outer0);
		ring_.indices.push_back(inner0);
		ring_.indices.push_back(outer1);

		// 三角形2
		ring_.indices.push_back(outer1);
		ring_.indices.push_back(inner0);
		ring_.indices.push_back(inner1);
	}

	ring_.vBuffer = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(VertexDate) * ring_.vertices.size());
	ring_.iBuffer = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(uint32_t) * ring_.indices.size());

	VertexDate* vData = nullptr;
	ring_.vBuffer->Map(0, nullptr, reinterpret_cast<void**>(&vData));
	std::memcpy(vData, ring_.vertices.data(), sizeof(VertexDate) * ring_.vertices.size());

	ring_.vbView.BufferLocation = ring_.vBuffer->GetGPUVirtualAddress();
	ring_.vbView.SizeInBytes = static_cast<UINT>(sizeof(VertexDate) * ring_.vertices.size());
	ring_.vbView.StrideInBytes = static_cast<UINT>(sizeof(VertexDate));

	uint32_t* iData = nullptr;
	ring_.iBuffer->Map(0, nullptr, reinterpret_cast<void**>(&iData));
	std::memcpy(iData, ring_.indices.data(), sizeof(uint32_t) * ring_.indices.size());

	ring_.ibView.BufferLocation = ring_.iBuffer->GetGPUVirtualAddress();
	ring_.ibView.Format = DXGI_FORMAT_R32_UINT;
	ring_.ibView.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * ring_.indices.size());
}

void ParticleManager::InitSphereVertex() {
	const float pi = 3.1415926535f;
	const uint32_t kSubdivision = 16;

	const float kLonEvery = (2.0f * pi) / static_cast<float>(kSubdivision);
	const float kLatEvery = pi / static_cast<float>(kSubdivision);

	// 頂点生成
	for (uint32_t latIndex = 0; latIndex <= kSubdivision; ++latIndex) {
		float lat = -pi / 2.0f + kLatEvery * latIndex;
		float v = 1.0f - float(latIndex) / float(kSubdivision); // 上がv=0, 下がv=1になるように

		for (uint32_t lonIndex = 0; lonIndex <= kSubdivision; ++lonIndex) {
			// 経度ループ用に +1 まで回す
			float lon = lonIndex * kLonEvery;
			float u = float(lonIndex) / float(kSubdivision); // 経度でuを算出（0〜1）

			float x = cosf(lat) * cosf(lon);
			float y = sinf(lat);
			float z = cosf(lat) * sinf(lon);

			sphere_.vertices.push_back({ {x, y, z, 1.0f},{u, v},{x, y, z} });
		}
	}

	// インデックス生成
	for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
			uint32_t row1 = latIndex * (kSubdivision + 1);
			uint32_t row2 = (latIndex + 1) * (kSubdivision + 1);

			uint32_t v0 = row1 + lonIndex;
			uint32_t v1 = row1 + lonIndex + 1;
			uint32_t v2 = row2 + lonIndex;
			uint32_t v3 = row2 + lonIndex + 1;

			sphere_.indices.push_back(v0);
			sphere_.indices.push_back(v2);
			sphere_.indices.push_back(v1);

			sphere_.indices.push_back(v1);
			sphere_.indices.push_back(v2);
			sphere_.indices.push_back(v3);
		}
	}

	sphere_.vBuffer = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(VertexDate) * sphere_.vertices.size());
	sphere_.iBuffer = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(uint32_t) * sphere_.indices.size());

	VertexDate* vData = nullptr;
	sphere_.vBuffer->Map(0, nullptr, reinterpret_cast<void**>(&vData));
	std::memcpy(vData, sphere_.vertices.data(), sizeof(VertexDate) * sphere_.vertices.size());

	sphere_.vbView.BufferLocation = sphere_.vBuffer->GetGPUVirtualAddress();
	sphere_.vbView.SizeInBytes = static_cast<UINT>(sizeof(VertexDate) * sphere_.vertices.size());
	sphere_.vbView.StrideInBytes = static_cast<UINT>(sizeof(VertexDate));

	uint32_t* iData = nullptr;
	sphere_.iBuffer->Map(0, nullptr, reinterpret_cast<void**>(&iData));
	std::memcpy(iData, sphere_.indices.data(), sizeof(uint32_t) * sphere_.indices.size());

	sphere_.ibView.BufferLocation = sphere_.iBuffer->GetGPUVirtualAddress();
	sphere_.ibView.Format = DXGI_FORMAT_R32_UINT;
	sphere_.ibView.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * sphere_.indices.size());
}

void ParticleManager::InitCylinderVertex() {
	const uint32_t kCylinderDivide = 32;
	const float kTopRadius = 1.0f;
	const float kBottomRadius = 1.0f;
	const float kHeight = 2.0f;
	const float radianPerDivide = 2.0f * std::numbers::pi_v<float> / float(kCylinderDivide);

	for (uint32_t i = 0; i <= kCylinderDivide; i++) {
		float angle = i * radianPerDivide;
		float sinA = std::sin(angle);
		float cosA = std::cos(angle);
		float u = float(i) / float(kCylinderDivide);

		// 下
		Vector3 posBottom = { cosA * kBottomRadius, 0.0f, sinA * kBottomRadius };
		Vector3 normal = { cosA, 0.0f, sinA };
		cylinder_.vertices.push_back({ {posBottom.x, posBottom.y, posBottom.z, 1.0f}, {u, 1.0f}, normal });

		// 上
		Vector3 posTop = { cosA * kTopRadius, kHeight, sinA * kTopRadius };
		cylinder_.vertices.push_back({ {posTop.x, posTop.y, posTop.z, 1.0f}, {u, 0.0f}, normal });
	}

	// インデックス生成
	for (uint32_t i = 0; i < kCylinderDivide; i++) {
		uint32_t bottom0 = i * 2;
		uint32_t top0 = bottom0 + 1;
		uint32_t bottom1 = bottom0 + 2;
		uint32_t top1 = bottom0 + 3;

		// 三角形1
		cylinder_.indices.push_back(bottom0);
		cylinder_.indices.push_back(top0);
		cylinder_.indices.push_back(bottom1);

		// 三角形2
		cylinder_.indices.push_back(bottom1);
		cylinder_.indices.push_back(top0);
		cylinder_.indices.push_back(top1);
	}

	cylinder_.vBuffer = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(VertexDate) * cylinder_.vertices.size());
	cylinder_.iBuffer = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(uint32_t) * cylinder_.indices.size());

	VertexDate* vData = nullptr;
	cylinder_.vBuffer->Map(0, nullptr, reinterpret_cast<void**>(&vData));
	std::memcpy(vData, cylinder_.vertices.data(), sizeof(VertexDate) * cylinder_.vertices.size());

	cylinder_.vbView.BufferLocation = cylinder_.vBuffer->GetGPUVirtualAddress();
	cylinder_.vbView.SizeInBytes = static_cast<UINT>(sizeof(VertexDate) * cylinder_.vertices.size());
	cylinder_.vbView.StrideInBytes = static_cast<UINT>(sizeof(VertexDate));

	uint32_t* iData = nullptr;
	cylinder_.iBuffer->Map(0, nullptr, reinterpret_cast<void**>(&iData));
	std::memcpy(iData, cylinder_.indices.data(), sizeof(uint32_t) * cylinder_.indices.size());

	cylinder_.ibView.BufferLocation = cylinder_.iBuffer->GetGPUVirtualAddress();
	cylinder_.ibView.Format = DXGI_FORMAT_R32_UINT;
	cylinder_.ibView.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * cylinder_.indices.size());
}

void ParticleManager::InitLightningVertex() {
	lightning_ = std::make_unique<Object3d>();
	lightning_->Create("lightning.obj");
}