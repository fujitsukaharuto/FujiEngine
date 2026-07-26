#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include "../IGPUParticleEmitter.h"
#include "Engine/Math/Matrix/MatrixCalculation.h"
#include "Engine/DX/FrameCount.h"

using Microsoft::WRL::ComPtr;


namespace Graphics {

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

	/// <summary>
	/// モデルの三角形の上からパーティクルを発生させるエミッター
	/// </summary>
	class MeshSurfaceEmitter : public IGPUEmitter {
	public:

		MeshSurfaceEmitter(DXC::DXCom* dx);
		void InitMeshData(const std::string& fileName, DXC::DXCom* dx, DXC::SRVManager* srv);

		void Update(float deltaTime) override;
		void Dispatch(ID3D12GraphicsCommandList* cmd,
			DXC::DXCom* dx, DXC::SRVManager* srv, const ParticleCSHandles& shared) override;
		void DebugGUI() override;
		void Save(const std::string& fileName) override;
		void Load(const std::string& fileName) override;

		// 一度だけエミット
		void Emit() override;
		bool IsEmit() const override { return isEmit_; }

		EmitterSurface& GetData() { return data_; }

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

		/// <summary>Save/Load 共通のフィールド列挙(Writer/Reader を渡すだけ)。実体は .cpp</summary>
		template<class Ar> void SerializeFields(Ar& ar);

		EmitterSurface data_;
		bool isEmit_ = false;

		char saveName_[64] = "default";
		EmitterSurface* dataGPU_[DXC::kFrameCount_];
		Microsoft::WRL::ComPtr<ID3D12Resource> resource_[DXC::kFrameCount_];
		Microsoft::WRL::ComPtr<ID3D12Resource> verticesResource;
		Microsoft::WRL::ComPtr<ID3D12Resource> indicesResource;
		Microsoft::WRL::ComPtr<ID3D12Resource> areasResource;
		uint32_t verticesIndex;
		uint32_t indicesIndex;
		uint32_t areasIndex;
		bool isOnceEmit_ = false;
	};

}
