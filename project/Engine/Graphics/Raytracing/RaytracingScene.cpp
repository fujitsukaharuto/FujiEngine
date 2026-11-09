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
	/// <summary>そのメッシュの見た目。渡された数が足りなければ最後のもの、1つも無ければ白</summary>
	/// <param name="vertexSrvIndex">当たった先の頂点を引くバッファ。スキンメッシュはスキニング後のもの</param>
	RayInstanceData LookOf(const std::vector<RayInstanceData>& looks, size_t meshIndex, const Mesh& mesh, uint32_t vertexSrvIndex) {
		RayInstanceData look = looks.empty() ? RayInstanceData{} : looks[(std::min)(meshIndex, looks.size() - 1)];
		// 抜きの印としきい値は呼び出し側が入れる
		look.flags &= kRayInstanceCutout | (0xFFu << kRayInstanceAlphaRefShift);
		if (vertexSrvIndex != Mesh::kInvalidSrvIndex && mesh.GetIndexSrvIndex() != Mesh::kInvalidSrvIndex) {
			look.vertexBuffer = vertexSrvIndex;
			look.indexBuffer = mesh.GetIndexSrvIndex();
			look.flags |= kRayInstanceHasGeometry | kRayInstanceHasNormal;
		}
		return look;
	}

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
	EnsureInstanceDataCapacity(0);
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
	retiredBlas_.clear();
	instances_.clear();
	instanceData_.clear();

	if (instanceDataCapacity_ > 0) {
		for (uint32_t i = 0; i < DXC::kFrameCount_; ++i) {
			instanceDataBuffer_[i]->Unmap(0, nullptr);
			instanceDataMapped_[i] = nullptr;
			instanceDataBuffer_[i].Reset();
			DXC::SRVManager::GetInstance()->Free(instanceDataSrvIndex_[i]);
		}
		instanceDataCapacity_ = 0;
	}

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
	instanceData_.clear();
	Logger::Log("RaytracingScene: all BLAS invalidated (model reloaded).\n");
}

void RaytracingScene::DebugGUI() {
#ifdef _DEBUGMODE
	if (!ImGui::CollapsingHeader("Raytracing", ImGuiTreeNodeFlags_DefaultOpen)) { return; }

	ImGui::Indent();
	if (!isAvailable_) {
		ImGui::TextColored(Core::ImGuiTextError(), "RayQuery: NOT supported");
		ImGui::Unindent();
		return;
	}

	ImGui::TextColored(Core::ImGuiTextOk(), "RayQuery: Supported");
	ImGui::Text("BLAS (モデル数)     : %zu", GetBlasCount());
	ImGui::Text("TLAS インスタンス数 : %u", GetInstanceCount());
	ImGui::Text("TLAS 容量           : %u", instanceCapacity_);
	if (tlasSrvIndex_ == kInvalidSrvIndex) {
		ImGui::TextColored(Core::ImGuiTextWarn(), "TLAS 未構築 (描画対象がまだ無い)");
	} else {
		ImGui::Text("TLAS SRV index      : %u", tlasSrvIndex_);
	}
	ImGui::Unindent();
#endif // _DEBUGMODE
}

void RaytracingScene::BeginFrame() {
	instances_.clear();
	instanceData_.clear();
	++frameNumber_;
	ReleaseUnusedSkinnedBlas();
}

void RaytracingScene::ReleaseUnusedSkinnedBlas() {
	for (auto it = skinnedBlasMap_.begin(); it != skinnedBlasMap_.end();) {
		if (frameNumber_ - it->second.lastUsedFrame > kReleaseDelayFrames) {
			it = skinnedBlasMap_.erase(it);
		} else {
			++it;
		}
	}
	std::erase_if(retiredBlas_, [this](const RetiredBlas& r) { return frameNumber_ - r.retiredFrame > kReleaseDelayFrames; });
}

void RaytracingScene::AddInstance(const Model* model, const Math::Matrix4x4& world, const std::vector<RayInstanceData>* looks, bool isTranslucent,
	bool hasCutout, bool isStatic) {
	if (!isAvailable_) { return; }

	const Blas* blas = EnsureBlas(model);
	if (blas == nullptr) { return; }

	D3D12_RAYTRACING_INSTANCE_DESC desc{};
	StoreTransform3x4(world, desc.Transform);
	// シェーダは「InstanceID + BLAS の中でのメッシュの番号」で見た目を引く
	desc.InstanceID = static_cast<UINT>(instanceData_.size());
	const std::vector<Mesh>& meshes = model->GetMeshes();
	for (size_t i = 0; looks && i < meshes.size(); ++i) {
		// EnsureBlas が BLAS に入れなかったメッシュは番号が詰まる
		if (meshes[i].GetVertexCount() == 0 || meshes[i].GetIndexCount() == 0) { continue; }
		instanceData_.push_back(LookOf(*looks, i, meshes[i], meshes[i].GetVertexSrvIndex()));
	}
	desc.InstanceMask = isTranslucent ? kInstanceMaskTranslucent : kInstanceMaskOpaque;
	if (isStatic) {
		desc.InstanceMask |= isTranslucent ? kInstanceMaskStaticTranslucent : kInstanceMaskStaticOpaque;
	}
	desc.InstanceContributionToHitGroupIndex = 0;
	// 不透明でないとすると影のレイがアルファを確かめる
	desc.Flags = (hasCutout && looks) ? D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE : D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
	desc.AccelerationStructure = blas->address;

	instances_.push_back(desc);
}

void RaytracingScene::AddSkinnedInstance(const RenderObject* key, const Model* model,
	const std::vector<SkinnedMesh>& skinnedMeshes, const Math::Matrix4x4& world,
	const std::vector<RayInstanceData>* looks, bool isTranslucent, bool hasCutout) {
	if (!isAvailable_ || key == nullptr || model == nullptr) { return; }

	const std::vector<Mesh>& meshes = model->GetMeshes();
	const size_t meshCount = (std::min)(meshes.size(), skinnedMeshes.size());
	if (meshCount == 0) { return; }

	// 頂点はスキニング後のものを指す。インデックスはポーズで変わらないので元のまま
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometries;
	geometries.reserve(meshCount);
	// BLAS に入れたメッシュぶんだけ、同じ並びで集める
	std::vector<RayInstanceData> geometryLooks;
	geometryLooks.reserve(meshCount);

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
		if (looks) {
			geometryLooks.push_back(LookOf(*looks, i, meshes[i], skinnedMeshes[i].GetSrvIndex()));
		}
	}

	if (geometries.empty()) { return; }

	// 形状が前のフレームと同じかを見分ける値。ポーズは含めない
	uint64_t signature = geometries.size();
	for (const D3D12_RAYTRACING_GEOMETRY_DESC& geometry : geometries) {
		for (uint64_t value : { geometry.Triangles.VertexBuffer.StartAddress, static_cast<uint64_t>(geometry.Triangles.VertexCount),
			geometry.Triangles.IndexBuffer, static_cast<uint64_t>(geometry.Triangles.IndexCount) }) {
			signature = (signature ^ value) * 0x100000001b3ull;
		}
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	// 毎フレーム動くので探索速度より構築時間を優先し、形状が同じ間は更新で済ませる
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD |
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.NumDescs = static_cast<UINT>(geometries.size());
	inputs.pGeometryDescs = geometries.data();

	// 大きさは毎回確かめる。キーのアドレスは使い回されるので、別のモデルの小さいバッファに当たることがある
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild{};
	dxcommon_->GetDevice5()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuild);
	if (prebuild.ResultDataMaxSizeInBytes == 0) { return; }
	const UINT64 bufferSize = AlignUp(prebuild.ResultDataMaxSizeInBytes, kASAlignment);
	const UINT64 scratchSize = AlignUp(
		(std::max)(prebuild.ScratchDataSizeInBytes, prebuild.UpdateScratchDataSizeInBytes), kASAlignment);

	SkinnedBlas& blas = skinnedBlasMap_[key];
	blas.lastUsedFrame = frameNumber_;
	bool isUpdate = blas.buffer && blas.signature == signature && blas.refitCount < kMaxSkinnedRefits;
	if (!blas.buffer || blas.bufferSize < bufferSize || blas.scratchSize < scratchSize) {
		// 前のフレームのTLASがまだ古い方を指しているかもしれないので、すぐには捨てない
		if (blas.buffer) {
			retiredBlas_.push_back({ std::move(blas), frameNumber_ });
			blas = SkinnedBlas{};
			blas.lastUsedFrame = frameNumber_;
		}
		blas.buffer = DXC::Helper::CreateAccelerationStructureResource(dxcommon_->GetDevice(), bufferSize);
		blas.scratch = DXC::Helper::CreateUAVResource(dxcommon_->GetDevice(), scratchSize);
		blas.address = blas.buffer->GetGPUVirtualAddress();
		blas.bufferSize = bufferSize;
		blas.scratchSize = scratchSize;
		isUpdate = false;

		Logger::Log(std::format("RaytracingScene: skinned BLAS created. geometries={}, size={} bytes\n",
			geometries.size(), prebuild.ResultDataMaxSizeInBytes));
	}

	blas.signature = signature;
	blas.refitCount = isUpdate ? blas.refitCount + 1 : 0;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{};
	buildDesc.Inputs = inputs;
	if (isUpdate) {
		// 同じバッファの上で更新する
		buildDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
		buildDesc.SourceAccelerationStructureData = blas.address;
	}
	buildDesc.DestAccelerationStructureData = blas.address;
	buildDesc.ScratchAccelerationStructureData = blas.scratch->GetGPUVirtualAddress();

	// スキニングと同じQueueに積むので、記録順で「スキニング済みの頂点を読む」ことが保証される
	ID3D12GraphicsCommandList4* list = dxcommon_->GetDXCommand()->GetASBuildContext()->GetList4();
	list->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
	// TLAS構築の前のバリアは BuildTlas でまとめて1回
	hasPendingBlasBuild_ = true;

	D3D12_RAYTRACING_INSTANCE_DESC desc{};
	StoreTransform3x4(world, desc.Transform);
	desc.InstanceID = static_cast<UINT>(instanceData_.size());
	instanceData_.insert(instanceData_.end(), geometryLooks.begin(), geometryLooks.end());
	desc.InstanceMask = isTranslucent ? kInstanceMaskTranslucent : kInstanceMaskOpaque;
	desc.InstanceContributionToHitGroupIndex = 0;
	desc.Flags = (hasCutout && looks) ? D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE : D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
	desc.AccelerationStructure = blas.address;

	instances_.push_back(desc);
}

void RaytracingScene::EnsureTlasCapacity(uint32_t instanceCount) {
	if (instanceCount <= instanceCapacity_ && instanceBuffer_[0]) { return; }

	// 増えたときだけ倍々で取り直す
	uint32_t newCapacity = (instanceCapacity_ == 0) ? 64 : instanceCapacity_;
	while (newCapacity < instanceCount) { newCapacity *= 2; }

	// 前のフレームの描画がまだ古いTLASを読んでいることがある。待たずに解放したりSRVを指し直すと、
	// 実行中の描画が解放済みの木や、まだ構築していない木を読む
	if (tlasBuffer_) {
		dxcommon_->Flush();
	}

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

void RaytracingScene::EnsureInstanceDataCapacity(uint32_t count) {
	if (count <= instanceDataCapacity_ && instanceDataCapacity_ > 0) { return; }

	uint32_t newCapacity = (instanceDataCapacity_ == 0) ? 64 : instanceDataCapacity_;
	while (newCapacity < count) { newCapacity *= 2; }

	// 前のフレームの描画がまだ古いバッファを読んでいることがある(TLAS と同じ理由)
	if (instanceDataCapacity_ > 0) {
		dxcommon_->Flush();
	}

	DXC::SRVManager* srvManager = DXC::SRVManager::GetInstance();
	for (uint32_t i = 0; i < DXC::kFrameCount_; ++i) {
		if (instanceDataCapacity_ > 0) {
			instanceDataBuffer_[i]->Unmap(0, nullptr);
		} else {
			instanceDataSrvIndex_[i] = srvManager->Allocate();
		}
		instanceDataBuffer_[i] = DXC::Helper::CreateBufferResource(
			dxcommon_->GetDevice(), sizeof(RayInstanceData) * newCapacity);
		instanceDataBuffer_[i]->Map(0, nullptr, reinterpret_cast<void**>(&instanceDataMapped_[i]));
		// シェーダは切っていても宣言ぶんを読みに来るので、中身の無い所も白で埋めておく
		std::fill_n(instanceDataMapped_[i], newCapacity, RayInstanceData{});
		srvManager->CreateStructuredSRV(instanceDataSrvIndex_[i], instanceDataBuffer_[i].Get(), newCapacity, sizeof(RayInstanceData));
	}

	instanceDataCapacity_ = newCapacity;
}

uint32_t RaytracingScene::GetInstanceDataSrvIndex() const {
	if (instanceDataCapacity_ == 0) { return kInvalidSrvIndex; }
	return instanceDataSrvIndex_[dxcommon_->GetNowFrameCount()];
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

	EnsureInstanceDataCapacity(static_cast<uint32_t>(instanceData_.size()));
	if (!instanceData_.empty()) {
		std::memcpy(instanceDataMapped_[frameIndex], instanceData_.data(), sizeof(RayInstanceData) * instanceData_.size());
	}

	// 描画と同じQueueなので記録順で完成が保証される。別Queueへ移すならフェンスが要る
	ID3D12GraphicsCommandList4* list = dxcommon_->GetDXCommand()->GetASBuildContext()->GetList4();
	if (hasPendingBlasBuild_) {
		// このフレームに積んだ BLAS の構築をまとめて待つ
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrier.UAV.pResource = nullptr;
		list->ResourceBarrier(1, &barrier);
		hasPendingBlasBuild_ = false;
	}
	RecordTlasBuild(list, instanceCount, frameIndex);
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
