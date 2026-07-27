#include "RootParam.h"
#include "Engine/GraphicPipeline/BasePipeline.h"
#include <cassert>

using namespace Graphics;


uint32_t RootParam::Resolve(Pipe type, const BasePipeline* pipeline) const {
	assert(pipeline);

	const size_t index = static_cast<size_t>(type);
	assert(index < cache_.size());

	if (cache_[index] != kUnresolved_) {
		return cache_[index];
	}

	const uint32_t rootIndex = pipeline->GetRootIndex(name_);
	cache_[index] = static_cast<uint16_t>(rootIndex);
	return rootIndex;
}
