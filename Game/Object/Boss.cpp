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
#include "Random.h"

#include <thread>
#include <numbers>
#include <cmath>
#include <algorithm>
#undef max
#undef min



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
	kMaxLife_ = life_;

	//調整項目
	const char* groupName = "boss";
	GlobalVariables::GetInstance()->CreateGroup(groupName);
	GlobalVariables::GetInstance()->AddItem(groupName, "moveSpeed", moveSpeed_);
	GlobalVariables::GetInstance()->AddItem(groupName, "life", life_);
}

Boss::~Boss(){
	Audio::GetInstance()->SoundStopWave(bossDamage);
	for (int i = 0; i < 6; i++){
		delete choruth[i];
	}
	for (int i = 0; i < 10; i++) {
		delete damage[i];
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


	for (int i = 0; i < 10; i++) {
		Object3d* newDama = new Object3d();
		newDama->Create("star.obj");
		damage.push_back(newDama);
	}


	emit.name = "bossHit";
	emit.count = 1;
	emit.animeData.lifeTime = 48;
	emit.RandomSpeed({-0.0f,0.0f}, {-0.0f,0.0f}, {-0.0f,0.0f});
	emit.RandomTranslate({-2.5f,-2.5f}, {-0.0f,0.0f}, {-8.5f,-8.5f});
	emit.animeData.startSize = {4.0f,4.0f};
	emit.animeData.endSize = {1.2f,1.2f};
	emit.animeData.type = SizeType::kReduction;

	emitCloud.name = "noteChange";
	emitCloud.count = 2;
	emitCloud.grain.lifeTime_ = 50;
	emitCloud.RandomSpeed({ 0.00f,0.07f }, { -0.00f,0.04f }, { -0.000f,0.000f });
	emitCloud.RandomTranslate({ 0.0f,0.0f }, { -0.1f,0.1f }, { -4.0f,-4.0f });
	emitCloud.grain.transform.scale = { 2.5f,2.5f,1.0f };


	ApplyGlobalVariables();

}

void Boss::Update(){
	// 後退タイマーの処理
	if (retreatTimer_ > 0){
		retreatTimer_--;  // タイマーをデクリメント

		// タイマーが0になった場合の処理
		if (retreatTimer_ == 0){
			// isHitCapsel_がtrueならmoveSpeed_を負の値に設定
			if (isHitCapsel_){
				moveSpeed_ = -fabs(moveSpeed_);
			} else{
				// それ以外の場合は正の速度に戻す
				moveSpeed_ = fabs(moveSpeed_);

				// 音声の停止とフラグのリセット
				Audio::GetInstance()->SoundStopWave(bossDamage);
				isPlay = false;
				isChorusu = false;
				isDamagePaticle = false;
			}


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

	if (isDamagePaticle) {
		for (int i = 0; i < 10; i++) {
			damage[i]->transform.translate += particleSpeed[i] * FPSKeeper::DeltaTime();
			particleSpeed[i].y -= 0.0175f * FPSKeeper::DeltaTime();
		}
	}

	Vector3 offset {-2.0f,0.0f,0.0f};
	// コライダー用のポジションを更新
	collider_->Update(GetCenterPos() + offset);

	Character::Update();

	//y軸回転を固定
	models_[0]->transform.rotate.y = Lerp(models_[0]->transform.rotate.y,3.14f,0.1f);

	models_[0]->transform.translate.x = std::max(models_[0]->transform.translate.x, -5.0f + Field::cameraScrollX_);
}

bool Boss::UpdateBegineGame(){
	// ボスの初期位置と目標位置を定義
	static float initialY = models_[0]->transform.translate.y;
	float targetY = 3.0f;
	models_[0]->transform.rotate.y = 4.0f;

	// 落下の状態を保持する変数
	static float fallSpeed = 8.0f; // 初期の落下速度
	float gravity = 30.0f; // 重力加速度（調整可能）
	float deltaTime = 0.0166f;

	// 一度だけ初期位置をセットする
	if (models_[0]->transform.translate.y == initialY){
		fallSpeed = 0.0f; // 初期化
	}

	// 落下の処理
	if (models_[0]->transform.translate.y > targetY){
		fallSpeed += gravity * deltaTime;
		models_[0]->transform.translate.y -= fallSpeed * deltaTime;

		if (models_[0]->transform.translate.y <= targetY){
			models_[0]->transform.translate.y = targetY;
			fallSpeed = 0.0f;
			CameraManager::GetInstance()->GetCamera()->SetShakeTime(60.0f);
			emitCloud.RandomSpeed({ 0.00f,0.07f }, { -0.00f,0.04f }, { -0.000f,0.000f });
			emitCloud.pos = GetCenterPos();
			emitCloud.pos.x += 2.0f;
			emitCloud.pos.y -= 5.0f;
			emitCloud.Burst();
			emitCloud.RandomSpeed({ -0.07f,0.00f }, { -0.00f,0.04f }, { -0.000f,0.000f });
			emitCloud.pos = GetCenterPos();
			emitCloud.pos.x -= 6.0f;
			emitCloud.pos.y -= 5.0f;
			emitCloud.Burst();
		}
	}

	// カメラズームイン、モーション、ズームアウト完了を追跡するフラグ
	static bool cameraZoomInComplete = false;
	static bool motionComplete = false;
	static bool cameraZoomOutComplete = false;

	if (models_[0]->transform.translate.y <= targetY){
		if (!cameraZoomInComplete){
			if (CameraManager::GetInstance()->EaseMoveCamera({-20.0f, 5.0f, -58.5f}, deltaTime)){
				cameraZoomInComplete = true;
			} else{
				return true;
			}
		}

		if (!motionComplete){
			if (UpdateExpansionAndContraction(deltaTime)){
				motionComplete = true;
			} else{
				return true;
			}
		}

		if (motionComplete && !cameraZoomOutComplete){
			if (CameraManager::GetInstance()->EaseMoveCamera({-11.5f, 5.6f, -63.5f}, deltaTime)){
				cameraZoomOutComplete = true;
			} else{
				return true;
			}
		}

		// 全ての処理が終了した際のリセット処理
		if (cameraZoomOutComplete){
			// 使用したstatic変数のリセット
			initialY = models_[0]->transform.translate.y;
			fallSpeed = 8.0f;
			cameraZoomInComplete = false;
			motionComplete = false;
			cameraZoomOutComplete = false;
			return false; // 完了
		}
	}

	Character::Update();
	return true;
}

bool Boss::UpdateExpansionAndContraction(float deltaTime){
	// 伸縮のアニメーションに使う変数
	static float animationTime = 0.0f;
	static int expansionCount = 0; // 伸縮の回数を保持
	float animationDuration = 1.0f; // アニメーションにかける時間（秒）
	float frequencyBoss = 1.0f; // 伸縮の速さ（調整可能）
	float scaleMin = 0.8f;  // 最小スケール
	float scaleMax = 1.2f;  // 最大スケール

	// アニメーションの進行度を更新
	animationTime += deltaTime;

	// 1回の伸縮アニメーションが終了した場合
	if (animationTime >= animationDuration){
		animationTime = 0.0f; // アニメーション時間をリセット
		expansionCount++; // 伸縮回数をカウント

		// 2回の伸縮が完了した場合、終了
		if (expansionCount >= 2){
			models_[0]->transform.scale.y = 1.0f; // スケールをリセット
			expansionCount = 0; // カウントをリセット
			return true; // アニメーションの完了を通知
		}
	}

	// 進行度を0〜1の範囲に正規化
	float normalizedTime = animationTime / animationDuration;

	// 伸縮のアニメーションを計算
	float sinVal = sin(normalizedTime * frequencyBoss * std::numbers::pi_v<float> *2.0f); // sin波による伸縮

	// スケールを更新
	float scale = scaleMin + (scaleMax - scaleMin) * (0.5f * (1.0f + sinVal)); // -1 ~ 1を0 ~ 1にマッピング
	models_[0]->transform.scale.y = scale;

	return false; // アニメーションがまだ完了していない
}



void Boss::Draw(){
	//モデルの描画
	Character::Draw();
	for (int i = 0; i < 6; i++){
		choruth[i]->Draw();
	}

	if (isDamagePaticle) {
		for (int i = 0; i < 10; i++) {
			damage[i]->Draw();
		}
	}

	float offset = 10.0f;
	float linePosX = models_[0]->transform.translate.x + offset;
	Field::fieldEndPosX = linePosX;
	//フィールドの終了線
	PrimitiveDrawer::GetInstance()->DrawLine3d({linePosX,12.0f,0.0f}, {linePosX, 0.0f, 0.0f}, {0.0f,0.0f,0.0f,1.0f});
}

void Boss::Move(){

	//前に進んでいつとき
	if (moveSpeed_ > 0){
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
	float waveScale = 1.0f + waveAmplitudeScale * sin(waveFrequency * localTime * 2.0f); // 1.0fを基準とする

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
		if (!isPlay) {
			Audio::GetInstance()->SoundPlayWave(bossDamage, 0.1f);
			isPlay = true;
		}


		
		int32_t lifePareto = kMaxLife_ / 4;
		// 残り4分の3の時
		if (lifePareto * 3 == life_) {
			models_[0]->SetTexture("boss_state1.png");
			// Particleを出す
			if (!isDamagePaticle) {
				particleSpeed.clear();
				for (int i = 0; i < 10; i++) {
					damage[i]->transform.translate = GetCenterPos();
					float randSize = Random::GetFloat(0.3f, 0.6f);
					damage[i]->transform.scale = { randSize,randSize ,randSize };
					particleSpeed.push_back(Random::GetVector3({ -0.4f,0.2f }, { 0.2f,0.5f }, { -0.3f,-0.2f }));
				}
				isDamagePaticle = true;
			}
		}
		// 残り4分の2の時
		if (lifePareto * 2 == life_) {
			models_[0]->SetTexture("boss_state2.png");
			// Particleを出す
			if (!isDamagePaticle) {
				particleSpeed.clear();
				for (int i = 0; i < 10; i++) {
					damage[i]->transform.translate = GetCenterPos();
					float randSize = Random::GetFloat(0.3f, 0.6f);
					damage[i]->transform.scale = { randSize,randSize ,randSize };
					particleSpeed.push_back(Random::GetVector3({ -0.4f,0.2f }, { 0.2f,0.5f }, { -0.3f,-0.2f }));
				}
				isDamagePaticle = true;
			}
		}
		// 残り4分の1の時
		if (lifePareto == life_) {
			models_[0]->SetTexture("boss_state3.png");
			// Particleを出す
			if (!isDamagePaticle) {
				particleSpeed.clear();
				for (int i = 0; i < 10; i++) {
					damage[i]->transform.translate = GetCenterPos();
					float randSize = Random::GetFloat(0.3f, 0.6f);
					damage[i]->transform.scale = { randSize,randSize ,randSize };
					particleSpeed.push_back(Random::GetVector3({ -0.4f,0.2f }, { 0.2f,0.5f }, { -0.3f,-0.2f }));
				}
				isDamagePaticle = true;
			}
		}

		// ボスを後退させる
		life_--;
		moveSpeed_ *= -1;  // ボスの後退速度を設定 (負の値で後退)

		retreatTimer_ = 60;  // 60フレーム（1秒間）後退

		if (!enemy->isFirst_){
			retreatTimer_ = 0;
			isHitCapsel_ = false;
		}
	}


	if (life_ == 0) {
		SceneManager::GetInstance()->SetClear(true);
		isClear = true;
		SceneManager::GetInstance()->ChangeScene("RESULT", 40.0f);
	}
}

Vector3 Boss::GetCenterPos() const{
	Vector3 offset = {1.0f, 3.0f, 1.8f};
	Vector3 worldPos = Transform(offset, models_[0]->GetMatWorld());
	return worldPos;
}
