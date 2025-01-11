#include "StrongEnemy.h"

///========================================================
///  初期化
///========================================================
void StrongEnemy::Initialize() {
	BaseEnemy::Initialize();

	OriginGameObject::SetModel("StrongEnemy.obj");

	damageParm_ = 0.2f;
}

///========================================================
/// 更新
///========================================================
void StrongEnemy::Update() {
	BaseEnemy::Update();
}

///========================================================
/// 描画
///========================================================
void StrongEnemy::Draw(Material* material) {
	BaseEnemy::Draw(material);
}
