#include "NormalEnemy.h"

///========================================================
///  初期化
///========================================================
void NormalEnemy::Initialize() {
	BaseEnemy::Initialize();

	OriginGameObject::CreateModel("NormalEnemy.obj");

}

///========================================================
/// 更新
///========================================================

void NormalEnemy::Update() {
	BaseEnemy::Update();
}

///========================================================
/// 描画
///========================================================

void NormalEnemy::Draw(Material* material) {
	BaseEnemy::Draw(material);
}
