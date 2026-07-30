#include "Engine/Graphics/Raytracing/RaytracingScene.h"

#include <algorithm>
#include <cassert>
#include <cstring>

#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/DXC/Command/DXCommand.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/DXC/Resource/SRVManager.h"
#include "Engine/Graphics/Model/Mesh/Mesh.h"
#include "Engine/Graphics/Model/Mesh/SkinnedMesh.h"
#include "Engine/Graphics/Model/Model.h"
#include "Engine/Logger/Logger.h"

using namespace Graphics;

namespace {

	// 加速構造とスクラッチは256バイト境界に載せる必要がある
	constexpr UINT64 kASAlignment = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;

	UINT64 AlignUp(UINT64 size, UINT64 alignment) {
		return (size + alignment - 1) & ~(alignment - 1);
	}

	/// <summary>ワールド行列をDXRのインスタンス行列(3x4)へ詰める</summary>
	/// <remarks>Matrix4x4 は行ベクトル、DXRは列ベクトルなので転置する</remarks>
	void StoreTransform3x4(const Math::Matrix4x4& world, FLOAT out[3][4]) {
		for (int row = 0; row < 3; ++row) {
			for (int col = 0; col < 4; ++col) {
				out[row][col] = world.m[col][row];
			}
		}
	}
}

void RaytracingScene::Initialize(DXC::DXCom* pDxcom) {
	dxcommon_ = pDxcom;

	// ここで弾けば以降の全メソッドが空振りする
	isAvailable_ = dxcommon_->IsRayQuerySupported();
	if (!isAvailable_) {
		Logger::Log("RaytracingScene: disabled (RayQuery is not supported on this device).\n");
		return;
	}

	// シェーダは gSceneTLAS を無条件に読むので、描画対象0でも空のTLASを作っておく。
	// 未初期化バッファをRayQueryに読ませると結果が不定になる
	EnsureTlasCapacity(0);
	RecordTlasBuild(dxcommon_->GetDXCommand()->GetImmediateList4(), 0, 0);
	dxcommon_->CommandExecution();

	Logger::Log("RaytracingScene: initialized.\n");
}

void RaytracingScene::Finalize() {
	for (uint32_t i = 0; i < DXC::kFrameCount_; ++i) {
		if (instanceMapped_[i]) {
			instanceBuffer_[i]->Unmap(0, nullptr);
			instanceMapped_[i] = nullptr;
		}
		instanceBuffer_[i].Reset();
	}
	tlasScratch_.Reset();
	tlasBuffer_.Reset();
	blasMap_.clear();
	skinnedBlasMap_.clear();
	instances_.clear();

	// SRVの返却はGPUが参照し終わってからでないといけない。
	// Finalize は DXCom::Flush 後に呼ばれる前提
	if (tlasSrvIndex_ != kInvalidSrvIndex) {
		DXC::SRVManager::GetInstance()->Free(tlasSrvIndex_);
		tlasSrvIndex_ = kInvalidSrvIndex;
	}
}

const RaytracingScene::Blas* RaytracingScene::EnsureBlas(const Model* model) {
	if (model == nullptr) { return nullptr; }

	auto it = blasMap_.find(model);
	if (it != blasMap_.end()) {
		return &it->second;
	}

	const std::vector<Mesh>& meshes = model->GetMeshes();
	if (meshes.empty()) { return nullptr; }

	// 影はマテリアル別の分岐が要らないので、全メッシュを1つのBLASにまとめる
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometries;
	geometries.reserve(meshes.size());

	for (const Mesh& mesh : meshes) {
		if (mesh.GetVertexCount() == 0 || mesh.GetIndexCount() == 0) { continue; }

		const D3D12_VERTEX_BUFFER_VIEW& vbv = mesh.GetVBV();
		const D3D12_INDEX_BUFFER_VIEW& ibv = mesh.GetIBV();

		D3D12_RAYTRACING_GEOMETRY_DESC geometry{};
		geometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		// OPAQUE にすると any-hit が省かれる
		geometry.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

		// pos は Vector4 だが、R32G32B32 + ストライド48 で w は読み飛ばされる
		geometry.Triangles.VertexBuffer.StartAddress = vbv.BufferLocation;
		geometry.Triangles.VertexBuffer.StrideInBytes = vbv.StrideInBytes;
		geometry.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		geometry.Triangles.VertexCount = static_cast<UINT>(mesh.GetVertexCount());
		geometry.Triangles.IndexBuffer = ibv.BufferLocation;
		geometry.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
		geometry.Triangles.IndexCount = static_cast<UINT>(mesh.GetIndexCount());
		geometry.Triangles.Transform3x4 = 0; // ローカル空間のまま作る

		geometries.push_back(geometry);
	}

	if (geometries.empty()) { return nullptr; }

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	// 静的メッシュは作り直さないので、構築時間より探索速度を優先する
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.NumDescs = static_cast<UINT>(geometries.size());
	inputs.pGeometryDescs = geometries.data();

	// 必要なサイズはドライバに訊く
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild{};
	dxcommon_->GetDevice5()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuild);
	if (prebuild.ResultDataMaxSizeInBytes == 0) { return nullptr; }

	Blas blas;
	blas.buffer = DXC::Helper::CreateAccelerationStructureResource(
		dxcommon_->GetDevice(), AlignUp(prebuild.ResultDataMaxSizeInBytes, kASAlignment));

	// スクラッチは構築中だけ要るのでローカルに置く
	Microsoft::WRL::ComPtr<ID3D12Resource> scratch = DXC::Helper::CreateUAVResource(
		dxcommon_->GetDevice(), AlignUp(prebuild.ScratchDataSizeInBytes, kASAlignment));

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{};
	buildDesc.Inputs = inputs;
	buildDesc.DestAccelerationStructureData = blas.buffer->GetGPUVirtualAddress();
	buildDesc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();

	// 初期化用の経路で同期実行する。戻った時点でスクラッチを解放してよい
	ID3D12GraphicsCommandList4* list = dxcommon_->GetDXCommand()->GetImmediateList4();
	list->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.UAV.pResource = blas.buffer.Get();
	list->ResourceBarrier(1, &barrier);

	dxcommon_->CommandExecution();

	blas.address = blas.buffer->GetGPUVirtualAddress();

	Logger::Log(std::format("RaytracingScene: BLAS built. geometries={}, size={} bytes\n",
		geometries.size(), prebuild.ResultDataMaxSizeInBytes));

	auto inserted = blasMap_.emplace(model, std::move(blas));
	return &inserted.first->second;
}

void RaytracingScene::InvalidateAllBlas() {
	if (blasMap_.empty()) { return; }

	// 呼び出し側が Flush 済みである前提
	blasMap_.clear();
	instances_.clear();
	Logger::Log("RaytracingScene: all BLAS invalidated (model reloaded).\n");
}

void RaytracingScene::DebugGUI() {
#ifdef _DEBUGMODE
	if (!ImGui::CollapsingHeader("Raytracing", ImGuiTreeNodeFlags_DefaultOpen)) { return; }

	ImGui::Indent();
	if (!isAvailable_) {
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "RayQuery: NOT supported");
		ImGui::TextWrapped("DXR Tier 1.1 と Shader Model 6.5 の両方が要ります。起動ログを確認してください");
		ImGui::Unindent();
		return;
	}

	ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "RayQuery: Supported");
	ImGui::Text("BLAS (モデル数)     : %zu", GetBlasCount());
	ImGui::Text("TLAS インスタンス数 : %u", GetInstanceCount());
	ImGui::Text("TLAS 容量           : %u", instanceCapacity_);
	if (tlasSrvIndex_ == kInvalidSrvIndex) {
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "TLAS 未構築 (描画対象がまだ無い)");
	} else {
		ImGui::Text("TLAS SRV index      : %u", tlasSrvIndex_);
	}
	ImGui::Unindent();
#endif // _DEBUGMODE
}

void RaytracingScene::BeginFrame() {
	instances_.clear();
}

void RaytracingScene::AddInstance(const Model* model, const Math::Matrix4x4& world) {
	if (!isAvailable_) { return; }

	const Blas* blas = EnsureBlas(model);
	if (blas == nullptr) { return; }

	D3D12_RAYTRACING_INSTANCE_DESC desc{};
	StoreTransform3x4(world, desc.Transform);
	desc.InstanceID = static_cast<UINT>(instances_.size());
	desc.InstanceMask = 0xFF;
	desc.InstanceContributionToHitGroupIndex = 0;
	desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
	desc.AccelerationStructure = blas->address;

	instances_.push_back(desc);
}

void RaytracingScene::AddSkinnedInstance(const RenderObject* key, const Model* model,
	const std::vector<SkinnedMesh>& skinnedMeshes, const Math::Matrix4x4& world) {
	if (!isAvailable_ || key == nullptr || model == nullptr) { return; }

	const std::vector<Mesh>& meshes = model->GetMeshes();
	const size_t meshCount = (std::min)(meshes.size(), skinnedMeshes.size());
	if (meshCount == 0) { return; }

	// 頂点はスキニング後のものを指す。インデックスはポーズで変わらないので元のまま
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometries;
	geometries.reserve(meshCount);

	for (size_t i = 0; i < meshCount; ++i) {
		if (meshes[i].GetIndexCount() == 0) { continue; }

		const D3D12_VERTEX_BUFFER_VIEW& vbv = skinnedMeshes[i].GetSkinnedVBV();
		const D3D12_INDEX_BUFFER_VIEW& ibv = meshes[i].GetIBV();
		if (vbv.BufferLocation == 0) { continue; }

		D3D12_RAYTRACING_GEOMETRY_DESC geometry{};
		geometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geometry.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
		geometry.Triangles.VertexBuffer.StartAddress = vbv.BufferLocation;
		geometry.Triangles.VertexBuffer.StrideInBytes = vbv.StrideInBytes;
		geometry.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		geometry.Triangles.VertexCount = static_cast<UINT>(meshes[i].GetVertexCount());
		geometry.Triangles.IndexBuffer = ibv.BufferLocation;
		geometry.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
		geometry.Triangles.IndexCount = static_cast<UINT>(meshes[i].GetIndexCount());
		geometry.Triangles.Transform3x4 = 0;

		geometries.push_back(geometry);
	}

	if (geometries.empty()) { return; }

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	// 毎フレーム作り直すので探索速度より構築時間を優先する
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.NumDescs = static_cast<UINT>(geometries.size());
	inputs.pGeometryDescs = geometries.data();

	SkinnedBlas& blas = skinnedBlasMap_[key];
	if (!blas.buffer) {
		// 頂点数はポーズで変わらないのでバッファは初回だけ確保すればよい
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild{};
		dxcommon_->GetDevice5()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuild);
		if (prebuild.ResultDataMaxSizeInBytes == 0) { return; }

		blas.buffer = DXC::Helper::CreateAccelerationStructureResource(
			dxcommon_->GetDevice(), AlignUp(prebuild.ResultDataMaxSizeInBytes, kASAlignment));
		blas.scratch = DXC::Helper::CreateUAVResource(
			dxcommon_->GetDevice(), AlignUp(prebuild.ScratchDataSizeInBytes, kASAlignment));
		blas.address = blas.buffer->GetGPUVirtualAddress();

		Logger::Log(std::format("RaytracingScene: skinned BLAS created. geometries={}, size={} bytes\n",
			geometries.size(), prebuild.ResultDataMaxSizeInBytes));
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{};
	buildDesc.Inputs = inputs;
	buildDesc.DestAccelerationStructureData = blas.address;
	buildDesc.ScratchAccelerationStructureData = blas.scratch->GetGPUVirtualAddress();

	// スキニングと同じQueueに積むので、記録順で「スキニング済みの頂点を読む」ことが保証される
	ID3D12GraphicsCommandList4* list = dxcommon_->GetDXCommand()->GetASBuildContext()->GetList4();
	list->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	// TLAS構築がこのBLASを読むので、完成を待たせる
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.UAV.pResource = blas.buffer.Get();
	list->ResourceBarrier(1, &barrier);

	D3D12_RAYTRACING_INSTANCE_DESC desc{};
	StoreTransform3x4(world, desc.Transform);
	desc.InstanceID = static_cast<UINT>(instances_.size());
	desc.InstanceMask = 0xFF;
	desc.InstanceContributionToHitGroupIndex = 0;
	desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
	desc.AccelerationStructure = blas.address;

	instances_.push_back(desc);
}

void RaytracingScene::EnsureTlasCapacity(uint32_t instanceCount) {
	if (instanceCount <= instanceCapacity_ && instanceBuffer_[0]) { return; }

	// 増えたときだけ倍々で取り直す
	uint32_t newCapacity = (instanceCapacity_ == 0) ? 64 : instanceCapacity_;
	while (newCapacity < instanceCount) { newCapacity *= 2; }

	for (uint32_t i = 0; i < DXC::kFrameCount_; ++i) {
		if (instanceMapped_[i]) {
			instanceBuffer_[i]->Unmap(0, nullptr);
			instanceMapped_[i] = nullptr;
		}
		instanceBuffer_[i] = DXC::Helper::CreateBufferResource(
			dxcommon_->GetDevice(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * newCapacity);
		instanceBuffer_[i]->Map(0, nullptr, reinterpret_cast<void**>(&instanceMapped_[i]));
	}

	instanceCapacity_ = newCapacity;

	// TLAS本体とスクラッチも最大インスタンス数で取り直す
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.NumDescs = newCapacity;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild{};
	dxcommon_->GetDevice5()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuild);

	tlasBuffer_ = DXC::Helper::CreateAccelerationStructureResource(
		dxcommon_->GetDevice(), AlignUp(prebuild.ResultDataMaxSizeInBytes, kASAlignment));
	tlasScratch_ = DXC::Helper::CreateUAVResource(
		dxcommon_->GetDevice(), AlignUp(prebuild.ScratchDataSizeInBytes, kASAlignment));

	// 作り直すとアドレスが変わるのでSRVも指し直す
	DXC::SRVManager* srvManager = DXC::SRVManager::GetInstance();
	if (tlasSrvIndex_ == kInvalidSrvIndex) {
		tlasSrvIndex_ = srvManager->Allocate();
	}
	srvManager->CreateAccelerationStructureSRV(tlasSrvIndex_, tlasBuffer_->GetGPUVirtualAddress());

	Logger::Log(std::format("RaytracingScene: TLAS capacity = {} instances.\n", newCapacity));
}

void RaytracingScene::BuildTlas() {
	if (!isAvailable_) { return; }

	// インスタンスが0でも構築する(未初期化のTLASは読ませられない)
	const uint32_t instanceCount = static_cast<uint32_t>(instances_.size());
	EnsureTlasCapacity(instanceCount);

	// 前フレームの構築がまだ読んでいる可能性があるので書き込み先を分ける
	const uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	if (instanceCount > 0) {
		std::memcpy(instanceMapped_[frameIndex], instances_.data(),
			sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceCount);
	}

	// 描画と同じQueueなので記録順で完成が保証される。別Queueへ移すならフェンスが要る
	RecordTlasBuild(dxcommon_->GetDXCommand()->GetASBuildContext()->GetList4(), instanceCount, frameIndex);
}

void RaytracingScene::RecordTlasBuild(ID3D12GraphicsCommandList4* list, uint32_t instanceCount, uint32_t frameIndex) {
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.NumDescs = instanceCount;
	inputs.InstanceDescs = instanceBuffer_[frameIndex]->GetGPUVirtualAddress();

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{};
	buildDesc.Inputs = inputs;
	buildDesc.DestAccelerationStructureData = tlasBuffer_->GetGPUVirtualAddress();
	buildDesc.ScratchAccelerationStructureData = tlasScratch_->GetGPUVirtualAddress();

	list->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	// これが無いと構築途中の木をシェーダが読む
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.UAV.pResource = tlasBuffer_.Get();
	list->ResourceBarrier(1, &barrier);
}
