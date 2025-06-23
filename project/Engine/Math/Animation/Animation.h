#pragma once
#include <map>
#include <string>
#include "NodeAnimation.h"

struct Animation {
	std::string name;
	float duration;
	std::map<std::string, NodeAnimation> nodeAnimations;
};