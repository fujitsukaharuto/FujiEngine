#include "StrongEnemy.h"

///========================================================
///  初期化
///========================================================
void StrongEnemy::Initialize() {
	BaseEnemy::Initialize();

	OriginGameObject::CreateModel("StrongEnemy.obj");


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
