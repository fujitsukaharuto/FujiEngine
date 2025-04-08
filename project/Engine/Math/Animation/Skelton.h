#pragma once
#include "Math/Matrix/MatrixCalculation.h"
#include <vector>
#include <optional>
#include <map>

struct Joint {
	QuaternioonTrans transform;
	Matrix4x4 loaclMatrix;
	Matrix4x4 skeletonSpaceMatrix;
	std::string name;
	std::vector<int32_t> children;
	int32_t index;
	std::optional<int32_t> parent;
};

struct Skeleton {
	int32_t root;
	std::map<std::string, int32_t> jointMap;
	std::vector<Joint> joints;
};