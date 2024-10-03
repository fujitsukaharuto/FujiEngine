#include "ModelManager.h"

ModelManager::~ModelManager() {
}

ModelManager* ModelManager::GetInstance() {
	static ModelManager instance;
	return &instance;
}
