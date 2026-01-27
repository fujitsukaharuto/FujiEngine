#pragma once
#include "Math/Matrix/MatrixCalculation.h"
#include <vector>
#include <optional>
#include <map>

/// <summary>
/// ジョイント構造体
/// </summary>
struct Joint {
	Math::QuaternionTrans transform;
	Math::Matrix4x4 localMatrix;
	Math::Matrix4x4 skeletonSpaceMatrix;
	std::string name;
	std::vector<int32_t> children;
	int32_t index;
	std::optional<int32_t> parent;
};

/// <summary>
/// スケルトン構造体
/// </summary>
struct Skeleton {
	int32_t root;
	std::map<std::string, int32_t> jointMap;
	std::vector<Joint> joints;
};