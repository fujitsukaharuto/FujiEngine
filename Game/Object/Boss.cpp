#include "Boss.h"
#include "Collision/SphereCollider.h"
#include "Collision/CollisionManager.h"
#include "FPSKeeper.h"
#include "Field/Field.h"
#include "Rendering/PrimitiveDrawer.h"
#include "Object/NoteEnemy.h"
#include "GlobalVariables/GlobalVariables.h"
#include "CameraManager.h"

#include <thread>
#include <numbers>

Boss::Boss() : Character(std::make_unique<SphereCollider>()), stopMoveTimer_(0){
	SphereCollider* sphereCollider = dynamic_cast< SphereCollider* >(collider_.get());
	if (sphereCollider){
		sphereCollider->radius_ = 7.0f;
	}

	sphereCollider->SetTypeID(static_cast< uint32_t >(CollisionTypeIdDef::kBoss));
	CollisionManager::GetInstance()->AddCollider(this);

	//調整項目
	const char* groupName = "boss";
	GlobalVariables::GetInstance()->CreateGroup(groupName);
	GlobalVariables::GetInstance()->AddItem(groupName, "moveSpeed", moveSpeed_);
}

Boss::~Boss(){
	for (int i = 0; i < 6; i++) {
		delete choruth[i];
	}
}

void Boss::Initialize(std::vector<Object3d*> models){
	Character::Initialize(models);

	testLong= Audio::GetInstance()->SoundLoadWave("testLongBGM.wav");

	for (int i = 0; i < 3; i++) {
		Object3d* newCho = new Object3d();
		newCho->Create("testChorus.obj");
		newCho->SetCameraParent(true);
		newCho->transform.translate.x = 2.0f + 1.5f * i;
		newCho->transform.translate.y = -8.0f;
		newCho->transform.translate.z = 20.0f;
		choruth.push_back(newCho);
	}
	for (int i = 0; i < 3; i++) {
		Object3d* newCho = new Object3d();
		newCho->Create("testChorus.obj");
		newCho->SetCameraParent(true);
		newCho->transform.translate.x = -2.0f + -1.5f * i;
		newCho->transform.translate.y = -8.0f;
		newCho->transform.translate.z = 20.0f;
		choruth.push_back(newCho);
	}

	emit.name = "bossHit";
	emit.count = 1;
	emit.animeData.lifeTime = 20;
	emit.RandomSpeed({ -0.0f,0.0f }, { -0.0f,0.0f }, { -0.0f,0.0f });
	emit.RandomTranslate({ -0.0f,0.0f }, { -0.0f,0.0f }, { -5.5f,-5.0f });
	emit.animeData.startSize = { 4.0f,4.0f };
	emit.animeData.endSize = { 0.2f,0.2f };
	emit.animeData.type = SizeType::kReduction;
}

void Boss::Update(){
#ifdef _DEBUG
	//調整項目の適用
	ApplyGlobalVariables();
#endif // _DEBUG

	if (stopMoveTimer_ > 0){
		stopMoveTimer_--;  // タイマーをデクリメント
		if (stopMoveTimer_ == 0){
			moveSpeed_ = originalSpeed_;  // タイマーが0になったら元の速度に戻す
			Audio::GetInstance()->SoundStopWave(testLong);
			isChorusu = false;
		}
	}

	Move();
	for (int i = 0; i < 6; i++) {
		if (isChorusu) {
			choruth[i]->transform.translate.y = Lerp(choruth[i]->transform.translate.y, -5.6f, 0.3f);
			if (choruth[i]->transform.translate.y > -5.75f) {
				choruthTime += FPSKeeper::DeltaTime();

				float sinVal = 0.0f;
				if (i >= 3) {
					sinVal = sin(frequency * choruthTime);
					float angle = amplitude * sinVal;
					choruth[i]->transform.rotate.z = angle;
				}
				else {
					sinVal = sin(frequency * -choruthTime);
					float angle = amplitude * sinVal;
					choruth[i]->transform.rotate.z = angle;
				}

				// サイズの変化を計算 (cos波でサイズを変える)
				float scaleMin = 0.7f;     // 最小サイズ
				float scaleMax = 1.2f;     // 最大サイズ
				float scaleRange = scaleMax - scaleMin;

				// sinが0のときにcosが最大サイズを返すように調整 (位相シフト)
				float scale = scaleMin + (scaleRange * fabs(sinVal));
				choruth[i]->transform.scale.y = scale;
			}
		}
		else {
			choruth[i]->transform.translate.y = Lerp(choruth[i]->transform.translate.y, -8.0f, 0.3f);
			choruthTime = 0.0f;
			choruth[i]->transform.rotate.z = 0.0f;
			choruth[i]->transform.scale.y = 1.0f;
		}
	}
	//コライダー用のポジションを更新
	collider_->Update(GetWorldPosition());

	Character::Update();
}

void Boss::Draw(){
	//モデルの描画
	Character::Draw();
	for (int i = 0; i < 6; i++) {
		choruth[i]->Draw();
	}
	float offset = 10.0f;
	float linePosX = models_[0]->transform.translate.x + offset;
	//フィールドの終了線
	PrimitiveDrawer::GetInstance()->DrawLine3d({linePosX,12.0f,0.0f}, {linePosX, 0.0f, 0.0f}, {0.0f,0.0f,0.0f,1.0f});
}


void Boss::Move(){
	// 移動速度が0の場合は移動しない（停止中）
	if (moveSpeed_ != 0.0f){
		models_[0]->transform.translate.x += moveSpeed_ * FPSKeeper::DeltaTime();
	}
}

void Boss::ApplyGlobalVariables(){
	const char* groupName = "boss";
	moveSpeed_ = GlobalVariables::GetInstance()->GetFloatValue(groupName, "moveSpeed");
}

void Boss::StopMoveForCollision(uint32_t time){
 	moveSpeed_ = 0.0f;            // 一時的に動きを止める
	stopMoveTimer_ = time;        // タイマーを設定
}


float Boss::GetMoveSpeed() const{
	return moveSpeed_;
}

void Boss::OnCollision(Character* other){
	uint32_t collisionType = other->GetCollider()->GetTypeID();

	if (collisionType == static_cast< uint32_t >(CollisionTypeIdDef::kNoteEnemy)){
		NoteEnemy* enemy = static_cast< NoteEnemy* >(other);
		uint32_t serialNum = enemy->GetSerialNumber();

		if (record_.CheckRecord(serialNum)){
			return;
		}

		//履歴に登録
		record_.AddRecord(serialNum);

		//エフェクトを描画
		StopMoveForCollision(120);
		emit.pos = other->GetCenterPos();
		emit.BurstAnime();
		CameraManager::GetInstance()->GetCamera()->SetShakeTime(40.0f);

		isChorusu = true;
		Audio::GetInstance()->SoundPlayWave(testLong);
	}
}
