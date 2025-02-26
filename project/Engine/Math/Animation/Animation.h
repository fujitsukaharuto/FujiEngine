#pragma once
#include <map>
#include <string>
#include "NodeAnimation.h"

struct Animation {
	float duration;
	std::map<std::string, NodeAnimation> nodeAnimations;
};