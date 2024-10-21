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
#include <algorithm>
#undef max


uint32_t Boss::nextSerialNumber = 0;

Boss::Boss() : Character(std::make_unique<SphereCollider>()), stopMoveTimer_(0){
	serialNumber_ = nextSerialNumber;
	++nextSerialNumber;
	SphereCollider* sphereCollider = dynamic_cast< SphereCollider* >(collider_.get());
	if (sphereCollider){
		sphereCollider->radius_ = 7.75f;
	}

	sphereCollider->SetTypeID(static_cast< uint32_t >(CollisionTypeIdDef::kBoss));
	CollisionManager::GetInstance()->AddCollider(this);

	//初期値として20
	life_ = 20;

	//調整項目
	const char* groupName = "boss";
	GlobalVariables::GetInstance()->CreateGroup(groupName);
	GlobalVariables::GetInstance()->AddItem(groupName, "moveSpeed", moveSpeed_);
	GlobalVariables::GetInstance()->AddItem(groupName, "life", life_);
}

Boss::~Boss(){
	for (int i = 0; i < 6; i++){
		delete choruth[i];
	}
}

void Boss::Initialize(std::vector<Object3d*> models){
	Character::Initialize(models);
	//コライダー用のポジションを更新
	bossDamage = Audio::GetInstance()->SoundLoadWave("testLongBGM.wav");

	for (int i = 0; i < 3; i++){
		Object3d* newCho = new Object3d();
		newCho->Create("chorus.obj");
		newCho->SetCameraParent(true);
		newCho->transform.translate.x = 2.0f + 2.25f * i;
		newCho->transform.translate.y = -10.0f;
		newCho->transform.translate.z = 25.0f - i * 1.0f;
		newCho->transform.rotate.y = 3.14f;
		choruth.push_back(newCho);
	}
	for (int i = 0; i < 3; i++){
		Object3d* newCho = new Object3d();
		newCho->Create("chorus.obj");
		newCho->SetCameraParent(true);
		newCho->transform.translate.x = -2.0f + -2.25f * i;
		newCho->transform.translate.y = -10.0f;
		newCho->transform.translate.z = 25.0f - i * 1.0f;
		newCho->transform.rotate.y = 3.14f;
		choruth.push_back(newCho);
	}

	emit.name = "bossHit";
	emit.count = 1;
	emit.animeData.lifeTime = 48;
	emit.RandomSpeed({ -0.0f,0.0f }, { -0.0f,0.0f }, { -0.0f,0.0f });
	emit.RandomTranslate({ -0.0f,0.0f }, { -0.0f,0.0f }, { -5.5f,-5.0f });
	emit.animeData.startSize = { 4.0f,4.0f };
	emit.animeData.endSize = { 1.2f,1.2f };
	emit.animeData.type = SizeType::kReduction;

#ifdef _DEBUG
	// 調整項目の適用
	ApplyGlobalVariables();
#endif // _DEBUG
}

void Boss::Update(){
	// 後退タイマーの処理
	if (retreatTimer_ > 0){
		retreatTimer_--;  // タイマーをデクリメント
		if (retreatTimer_ == 0){
			moveSpeed_ = fabs(moveSpeed_);  // 後退が終わったら正の速度に戻す
			Audio::GetInstance()->SoundStopWave(bossDamage);
			isChorusu = false;
		}
	}

	Move();

	// コーラスの更新処理
	for (int i = 0; i < 6; i++){
		if (isChorusu){
			// コーラスの移動とアニメーションの処理
			choruth[i]->transform.translate.y = Lerp(choruth[i]->transform.translate.y, -6.3f, 0.3f);
			if (choruth[i]->transform.translate.y > -6.45f){
				choruthTime += FPSKeeper::DeltaTime();

				float sinVal = 0.0f;
				if (i >= 3){
					sinVal = sin(frequency * choruthTime);
					float angle = amplitude * sinVal;
					choruth[i]->transform.rotate.z = angle;
				} else{
					sinVal = sin(frequency * -choruthTime);
					float angle = amplitude * sinVal;
					choruth[i]->transform.rotate.z = angle;
				}

				float scaleMin = 0.7f;  // 最小サイズ
				float scaleMax = 1.2f;  // 最大サイズ
				float scaleRange = scaleMax - scaleMin;

				float scale = scaleMin + (scaleRange * fabs(sinVal));
				choruth[i]->transform.scale.y = scale;
			}
		} else{
			choruth[i]->transform.translate.y = Lerp(choruth[i]->transform.translate.y, -10.0f, 0.3f);
			choruthTime = 0.0f;
			choruth[i]->transform.rotate.z = 0.0f;
			choruth[i]->transform.scale.y = 1.0f;
		}
	}

	// コライダー用のポジションを更新
	collider_->Update(GetCenterPos());

	Character::Update();

	models_[0]->transform.translate.x = std::max(models_[0]->transform.translate.x, 0.0f + Field::scrollX_);
}


void Boss::Draw(){
	//モデルの描画
	Character::Draw();
	for (int i = 0; i < 6; i++){
		choruth[i]->Draw();
	}
	float offset = 10.0f;
	float linePosX = models_[0]->transform.translate.x + offset;
	Field::fieldEndPosX = linePosX;
	//フィールドの終了線
	PrimitiveDrawer::GetInstance()->DrawLine3d({linePosX,12.0f,0.0f}, {linePosX, 0.0f, 0.0f}, {0.0f,0.0f,0.0f,1.0f});
}


void Boss::Move(){
	// 移動速度が0の場合は移動しない（停止中）
	models_[0]->transform.translate.x += moveSpeed_ * FPSKeeper::DeltaTime();
}

void Boss::ApplyGlobalVariables(){
	const char* groupName = "boss";
	moveSpeed_ = GlobalVariables::GetInstance()->GetFloatValue(groupName, "moveSpeed");
	life_ = GlobalVariables::GetInstance()->GetIntValue(groupName, "life");
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

		// 履歴に登録
		record_.AddRecord(serialNum);

		// エフェクトを描画
		emit.pos = other->GetCenterPos();
		emit.BurstAnime();
		CameraManager::GetInstance()->GetCamera()->SetShakeTime(40.0f);

		isChorusu = true;
		Audio::GetInstance()->SoundPlayWave(bossDamage);

		// ボスを後退させる
		moveSpeed_ *= -1;  // ボスの後退速度を設定 (負の値で後退)
		retreatTimer_ = 90;  // 60フレーム（1秒間）後退
	}
}

Vector3 Boss::GetCenterPos() const{
	Vector3 offset = {0.0f, 3.0f, 1.8f};
	Vector3 worldPos = Transform(offset, models_[0]->GetMatWorld());
	return worldPos;
}
