#include "Boss.h"
#include "Collision/SphereCollider.h"
#include "Collision/CollisionManager.h"
#include "FPSKeeper.h"
#include "Field/Field.h"
#include "Rendering/PrimitiveDrawer.h"
#include "Object/NoteEnemy.h"
#include "GlobalVariables/GlobalVariables.h"
#include "CameraManager.h"
#include "SceneManager.h"

#include <thread>
#include <numbers>
#include <cmath>
#include <algorithm>
#undef max



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
	emit.RandomTranslate({ -2.5f,-2.5f }, { -0.0f,0.0f }, { -8.5f,-8.5f });
	emit.animeData.startSize = { 4.0f,4.0f };
	emit.animeData.endSize = { 1.2f,1.2f };
	emit.animeData.type = SizeType::kReduction;


	ApplyGlobalVariables();

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

	Vector3 offset {-2.0f,0.0f,0.0f};
	// コライダー用のポジションを更新
	collider_->Update(GetCenterPos() + offset);

	Character::Update();

	models_[0]->transform.translate.x = std::max(models_[0]->transform.translate.x, -2.0f + Field::cameraScrollX_);
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

	//前に進んでいつとき
	if (moveSpeed_>0){
		MovementMotion();
		models_[0]->transform.rotate.x = 0.0f;
	} else{
		//攻撃を受けて後ろに下がるときは回転をリセット
		models_[0]->transform.rotate.z = 0.0f;
		models_[0]->transform.rotate.x = -0.3f;
	}

	// 移動速度が0の場合は移動しない（停止中）
	models_[0]->transform.translate.x += moveSpeed_ * FPSKeeper::DeltaTime();

	models_[0]->UpdateWorldMat();
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

void Boss::MovementMotion(){
	// ボスの揺れに関連する処理
	static float localTime = 0.0f;  // ローカルの時間変数を用意
	float waveFrequency = 0.07f;    // 揺れの周波数
	//float waveAmplitude = 0.15f;    // 揺れの振れ幅

	float waveAmplitudeScale = 0.2f; // 縦の伸縮の振れ幅

	// ローカル時間を増加させる
	localTime += FPSKeeper::DeltaTime();

	// 時間に基づいてz軸の揺れをsin関数で計算
	//float waveValue = waveAmplitude * sin(waveFrequency * localTime);
	float waveScale = 1.0f + waveAmplitudeScale * sin(waveFrequency * localTime*2.0f); // 1.0fを基準とする

	//models_[0]->transform.rotate.z = waveValue;
	models_[0]->transform.scale.y = waveScale; // 元のスケールから伸縮するように調整
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
		life_--;
		moveSpeed_ *= -1;  // ボスの後退速度を設定 (負の値で後退)
		retreatTimer_ = 90;  // 60フレーム（1秒間）後退
	}

	if (life_ == 0) {
		SceneManager::GetInstance()->SetClear(true);
		SceneManager::GetInstance()->ChangeScene("RESULT", 40.0f);
	}
}

Vector3 Boss::GetCenterPos() const{
	Vector3 offset = {1.0f, 3.0f, 1.8f};
	Vector3 worldPos = Transform(offset, models_[0]->GetMatWorld());
	return worldPos;
}
