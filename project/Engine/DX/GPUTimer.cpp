#include "GPUTimer.h"
#include <cassert>

using namespace DXC;

void GPUTimer::Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue, uint32_t maxTimerCount) {
	assert(device);
	assert(commandQueue);

	maxTimerCount_ = maxTimerCount;
	gpuFrequency_ = 0;

	// フレーム数 * 1フレームあたりの最大計測数 * (開始+終了)
	uint32_t totalQueryCount = kFrameCount_ * maxTimerCount_ * kTimestampsPerTimer;

	// 1. Query Heap 作成
	D3D12_QUERY_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
	heapDesc.Count = totalQueryCount;
	heapDesc.NodeMask = 0;

	HRESULT hr = device->CreateQueryHeap(&heapDesc, IID_PPV_ARGS(&queryHeap_));
	assert(SUCCEEDED(hr));

	// 2. Readback Buffer 作成
	size_t bufferSize = sizeof(uint64_t) * totalQueryCount;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_READBACK;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Alignment = 0;
	resDesc.Width = bufferSize;
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		hr = device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&queryBuffer_[i])
		);
	}
	assert(SUCCEEDED(hr));

	// 3. 周波数取得
	hr = commandQueue->GetTimestampFrequency(&gpuFrequency_);
	assert(SUCCEEDED(hr));
}

void GPUTimer::Finalize() {
	queryHeap_.Reset();
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		queryBuffer_[i].Reset();
	}
}

void GPUTimer::Begin(ID3D12GraphicsCommandList* list, uint32_t frameIndex, uint32_t timerId) {
	assert(timerId < maxTimerCount_);
	// インデックス計算: [Frameオフセット] + [Timerオフセット] + [Start]
	uint32_t index = (frameIndex * maxTimerCount_ + timerId) * kTimestampsPerTimer;
	list->EndQuery(queryHeap_.Get(), D3D12_QUERY_TYPE_TIMESTAMP, index);
}

void GPUTimer::End(ID3D12GraphicsCommandList* list, uint32_t frameIndex, uint32_t timerId) {
	assert(timerId < maxTimerCount_);
	// インデックス計算: [Frameオフセット] + [Timerオフセット] + [End]
	uint32_t index = (frameIndex * maxTimerCount_ + timerId) * kTimestampsPerTimer + 1;
	list->EndQuery(queryHeap_.Get(), D3D12_QUERY_TYPE_TIMESTAMP, index);
}

void GPUTimer::Resolve(ID3D12GraphicsCommandList* list, uint32_t frameIndex, uint32_t timerId) {
	assert(timerId < maxTimerCount_);

	// このタイマーの開始インデックス
	uint32_t startIndex = (frameIndex * maxTimerCount_ + timerId) * kTimestampsPerTimer;

	// Resolveするのはこのタイマーの2つ分(Start/End)だけ
	uint32_t count = kTimestampsPerTimer;
	uint64_t dstOffset = startIndex * sizeof(uint64_t);

	list->ResolveQueryData(
		queryHeap_.Get(),
		D3D12_QUERY_TYPE_TIMESTAMP,
		startIndex,
		count,
		queryBuffer_[frameIndex].Get(),
		dstOffset
	);
}

double GPUTimer::GetElapsedMS(uint32_t frameIndex, uint32_t timerId) {
	assert(timerId < maxTimerCount_);

	uint64_t* data = nullptr;
	D3D12_RANGE readRange = {};
	// 全体をマップして安全に読む
	// (最適化するならResolveした範囲だけMapするのもありだが、Readbackなので全体でもコストは低い)
	readRange.Begin = 0;
	readRange.End = sizeof(uint64_t) * kFrameCount_ * maxTimerCount_ * kTimestampsPerTimer;

	HRESULT hr = queryBuffer_[frameIndex]->Map(0, &readRange, reinterpret_cast<void**>(&data));
	if (FAILED(hr)) {
		return 0.0;
	}

	uint32_t index = (frameIndex * maxTimerCount_ + timerId) * kTimestampsPerTimer;
	uint64_t startTick = data[index];
	uint64_t endTick = data[index + 1];

	queryBuffer_[frameIndex]->Unmap(0, nullptr);

	// どちらかが0なら計測されていない or バグ
	if (startTick == 0 || endTick == 0) {
		return 0.0;
	}

	if (endTick < startTick) {
		return 0.0;
	}

	uint64_t delta = endTick - startTick;
	return (static_cast<double>(delta) / static_cast<double>(gpuFrequency_)) * 1000.0;
}