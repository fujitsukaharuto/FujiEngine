#include "Object/Player.h"
#include "Input.h"

#include "Collision/CollisionManager.h"
#include "Collision/SphereCollider.h"
#include "Collision/BoxCollider.h"
#include "GlobalVariables/GlobalVariables.h"
#include "Object/Boss.h"
#include "Object/Obstacle.h"
#include "Object/NoteEnemy.h"
#include "MatrixCalculation.h"
#include "SceneManager.h"
#include "Field/Field.h"
#include "CameraManager.h"

#include <algorithm>
#undef max
#undef min

Player::Player() : Character(std::make_unique<SphereCollider>()){
	SphereCollider* sphereCollider = dynamic_cast< SphereCollider* >(collider_.get());
	if (sphereCollider){
		sphereCollider->radius_ = 1.0f;
		sphereCollider->position_ = {100.0f, 0.0f, 0.0f};
	}

	//識別id
	sphereCollider->SetTypeID(static_cast< uint32_t >(CollisionTypeIdDef::kPlayer));
	CollisionManager::GetInstance()->AddCollider(this);


	life_ = 4;

	//調整項目
	const char* groupName = "player";
	GlobalVariables::GetInstance()->CreateGroup(groupName);
	GlobalVariables::GetInstance()->AddItem(groupName, "life", life_);
}

Player::~Player(){

	for (size_t i = 0; i < 4; i++){
		if (lifeSprite_[i]){
			delete lifeSprite_[i];

		}
	}
}

void Player::Initialize(std::vector<Object3d*> Object3ds){
	Character::Initialize(Object3ds);

	junpSE_ = Audio::GetInstance()->SoundLoadWave("jump.wav");
	damageSE_ = Audio::GetInstance()->SoundLoadWave("damage.wav");

	models_[0]->transform.translate = {12.0f,0.0f,0.0f};
	models_[0]->transform.rotate = {0.0f,frontRotateY,0.0f};
	models_[0]->transform.rotate = {0.0f,frontRotateY,0.0f};

	emit.name = "playerHit";
	emit.count = 1;
	emit.animeData.lifeTime = 20;
	emit.RandomSpeed({-0.00f,0.00f}, {-0.00f,0.00f}, {0.0f,0.0f});
	emit.RandomTranslate({-1.5f,-1.5f}, {0.0f,0.0f}, {-8.0f,-8.0f});
	emit.animeData.startSize = {2.0f,2.0f};
	emit.animeData.endSize = {2.0f,2.0f};

	life_ = 4;


	///////////////////////////////////////////////////////////////
	//スプライトの初期化
	for (size_t i = 0; i < 4; i++){
		lifeSprite_[i] = new Sprite();
		lifeSprite_[i]->Load("heart.png");
		lifeSprite_[i]->SetPos({1500.0f + (i * 80.0f), 190.0f, 0.0f});
		lifeSprite_[i]->SetSize({64.0f,64.0f});
	}
}

void Player::Update(){
	collider_->Update(GetCenterPos());

	// ノックバック中の処理
	if (isKnockedBack_){
		knockbackTimer_ -= 0.016f; // タイマーを減らす (例: 60FPSで1フレーム = 0.016秒)

		// 重力を適用
		const float kGravity = -1.0f; // 重力の値
		const float kDeltaTime = 0.016f; // フレーム間の時間 (60FPS想定)
		velocity_.y += kGravity * kDeltaTime; // 重力をY軸方向に加算

		// Z軸の回転を追加 (ノックバック中のみ)
		const float kRotationSpeed = 0.16f; // Z軸回転の速度

		if (knockbackDirection_ >= 1){
			models_[0]->transform.rotate.z -= kRotationSpeed; // Z軸を回転
		} else{
			models_[0]->transform.rotate.z += kRotationSpeed; // Z軸を回転
		}

		// 地面に着いたらノックバックを終了
		if (models_[0]->transform.translate.y <= 0.0f && !isKnockedBack_){
			models_[0]->transform.translate.y = 0.0f;
			velocity_.y = 0.0f; // Y軸速度をリセットして地面に着いた状態にする
			isKnockedBack_ = false; // ノックバックを終了
			models_[0]->transform.rotate.z = 0.0f; // Z軸の回転をリセット
		}


		// ノックバックが終了したら通常状態に戻す
		if (knockbackTimer_ <= 0.0f){
			isKnockedBack_ = false;
			record_.Clear();//接触履歴をリセット

			knockbackTimer_ = 0.0f;
			models_[0]->transform.rotate.z = 0.0f; // Z軸の回転をリセット
		}
	} else{
		// 通常の動作 (ジャンプと移動)
		this->Jump();
		this->Move();
	}

	//時間経過で接触履歴をリセット(0.7秒)
	record_.ClearAfterTime(0.7f);

	models_[0]->transform.translate += velocity_;

	//地面より下に行かないように
	models_[0]->transform.translate.y = std::max(models_[0]->transform.translate.y, 0.0f);
	models_[0]->transform.translate.x = std::min(models_[0]->transform.translate.x, Field::cameraScrollX_ + 45.0f);

	//生存フラグの管理
	Character::Update();
}

float EaseOut(float start, float end, float t){
	t = 1.0f - pow(1.0f - t, 3.0f);  // Ease out
	return start + (end - start) * t;
}

void Player::UpdateUi(){
	// 目標位置
	Vector3 targetPosBase = {800.0f, 190.0f, 0.0f};
	float delayBetweenSprites = 5.0f;  // 各スプライトの移動の遅延時間（秒）
	float duration = 18.0f;  // 移動にかかる時間（秒）

	static float elapsedTime = 0.0f;  // 経過時間を保持
	static bool animationComplete = false; // アニメーションが完了したかどうかを追跡

	// アニメーションが完了している場合は何もしない
	if (animationComplete) return;

	// フレーム間の時間を加算
	elapsedTime += FPSKeeper::DeltaTime();

	for (size_t i = 0; i < lifeSprite_.size(); i++){
		// 各スプライトごとに、遅延時間を考慮して開始
		float startTime = i * delayBetweenSprites;

		// スプライトの移動を開始するか確認
		if (elapsedTime > startTime){
			// 経過時間から開始時間を引いて補間率を計算
			float t = (elapsedTime - startTime) / duration;
			if (t > 1.0f) t = 1.0f;  // tが1を超えないようにする

			// 初期位置を画面外（仮の値 -100.0f）に設定
			Vector3 initialPos = {1500.0f, targetPosBase.y, targetPosBase.z};
			Vector3 targetPos = {targetPosBase.x + (i * 80.0f), targetPosBase.y, targetPosBase.z};

			// EaseOut関数でX軸の位置を補間
			float newX = EaseOut(initialPos.x, targetPos.x, t);
			lifeSprite_[i]->SetPos({newX, targetPos.y, targetPos.z});
		}
	}

	// すべてのスプライトの位置が移動完了したかチェック
	if (elapsedTime > (lifeSprite_.size() - 1) * delayBetweenSprites + duration){
		animationComplete = true;
	}
}



void Player::Draw(){

	if (isAlive_){
		if (!isKnockedBack_){
			Character::Draw();
		} else{
			// ノックバック中はちらちらする処理
			static float flickerTimer = 0.0f;
			float flickerDuration = 0.1f; // ちらちらする間隔（秒単位、調整可能）

			flickerTimer += 0.0166f; // deltaTimeは時間の経過を表す変数

			// ちらちらのタイミングで描画
			if (static_cast< int >(flickerTimer / flickerDuration) % 2 == 0){
				Character::Draw(); // ちらちらのタイミングでのみ描画する
			}

			// タイマーをリセットしてループさせる
			if (flickerTimer >= flickerDuration * 2.0f){
				flickerTimer = 0.0f;
			}
		}
	}


}

void Player::Move(){
	// ノックバック中は通常の移動を無効化
	if (!isKnockedBack_){
		float targetRotateY = models_[0]->transform.rotate.y;  // 現在の回転角度を保持
		float rotationSpeed = 0.1f;  // 回転の補間スピード

		// 移動の方向を決定
		if (Input::GetInstance()->PushKey(DIK_A)){
			targetRotateY = backRotateY;
			moveSpeed_ = -0.2f;
		} else if (Input::GetInstance()->PushKey(DIK_D)){
			targetRotateY = frontRotateY;
			moveSpeed_ = 0.2f;
		} else{
			moveSpeed_ = 0.0f;
		}

		// 現在の回転角度をターゲット角度に向かって補間する
		models_[0]->transform.rotate.y = LerpShortAngle(models_[0]->transform.rotate.y, targetRotateY, rotationSpeed);

		// 移動速度の設定
		velocity_.x = moveSpeed_;
	}
}


void Player::Jump(){
	const float kGravity = -1.0f; // 要調整
	const float kDeltaTime = 0.016f;

	// ジャンプ開始
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)){
		isJumping_ = true;
		velocity_.y = 0.3f;
		Audio::GetInstance()->SoundPlayWave(junpSE_, 0.07f);
	}

	// ジャンプ中の挙動
	if (isJumping_){
		velocity_.y += kGravity * kDeltaTime;

		// 地面に着いたらジャンプリセット
		if (models_[0]->transform.translate.y < 0.0f){
			models_[0]->transform.translate.y = 0.0f;
			isJumping_ = false;
			velocity_ = {0.0f, 0.0f, 0.0f};
		}
	}
}

Vector3 Player::GetCenterPos() const{
	Vector3 offset = {0.0f, 0.5f, 0.0f};
	Vector3 worldPos = Transform(offset, models_[0]->GetMatWorld());
	return worldPos;
}

void Player::OnCollision(Character* other){
	uint32_t collisionType = other->GetCollider()->GetTypeID();

	// 音符に変わる敵と衝突したとき
	if (collisionType == static_cast< uint32_t >(CollisionTypeIdDef::kNoteEnemy)){
		NoteEnemy* noteEnemy = static_cast< NoteEnemy* >(other);
		SphereCollider* enemyCollider = static_cast< SphereCollider* >(other->GetCollider());
		SphereCollider* playerCollider = static_cast< SphereCollider* >(this->GetCollider());

		Vector3 playerPos = playerCollider->GetPosition();
		Vector3 enemyPos = enemyCollider->GetPosition();
		float playerRadius = playerCollider->GetRadius();
		float enemyRadius = enemyCollider->GetRadius();

		uint32_t serialNum = noteEnemy->GetSerialNumber();

		if (record_.CheckRecord(serialNum)){
			return;
		}

		// 履歴に登録
		record_.AddRecord(serialNum);

		// プレイヤーがまだ空中にいるかどうかのチェック
		if (isJumping_){
			Vector3 collisionNormal = playerPos - enemyPos;
			float distance = collisionNormal.Lenght();

			collisionNormal.Normalize();

			// 反射時の速度減衰率
			const float kReflectionDamping = 0.5f;

			Vector3 playerVelocity = velocity_;

			// 速度ベクトルを反射し、減衰させる
			Vector3 reflectedVelocity = playerVelocity - 2 * collisionNormal * playerVelocity.Dot(collisionNormal);
			reflectedVelocity *= kReflectionDamping;

			// Y軸方向の速度が大きくなりすぎないように上限を設定
			const float kMaxVelocityY = 0.3f;
			reflectedVelocity.y = std::min(reflectedVelocity.y, kMaxVelocityY);

			reflectedVelocity.z = 0.0f;
			// プレイヤーの速度を更新
			velocity_ = reflectedVelocity;

			// プレイヤーを敵から少し離れた位置に移動させてめり込みを防止
			float penetrationDepth = (playerRadius + enemyRadius) - distance;
			playerPos += collisionNormal * (penetrationDepth * 0.5f);
			playerCollider->SetPosition(playerPos);
		}

		// 同じ高さで衝突した場合の処理
		if (std::abs(playerPos.y - enemyPos.y) <= 1.0f){

			//音符じゃないときかつノックバック中は攻撃を受ける
			if (!noteEnemy->GetIsChangedNote() && !isKnockedBack_){
				// ノックバック処理
				 // プレイヤーと敵のX座標の差を計算して、ノックバックの方向を決定
				float xDifference = playerPos.x - enemyPos.x;
				float direction = (xDifference > 0) ? 1.0f : -1.0f;

				const float kNoteEnemyCollisionXVelocity = 0.25f * direction;  // 水平方向の速度
				const float kNoteEnemyCollisionYVelocity = 0.35f;  // 上方向の速度

				// X方向の速度を設定して横に飛ばす
				velocity_.x = kNoteEnemyCollisionXVelocity;
				// Y方向の速度を設定してジャンプさせる
				velocity_.y = kNoteEnemyCollisionYVelocity;

				// プレイヤーを敵から少し押し出す
				playerPos.x += 0.5f * direction;
				playerCollider->SetPosition(playerPos);

				// ノックバック状態に設定
				isKnockedBack_ = true;
				knockbackTimer_ = 0.5f;  // ノックバックの継続時間を設定

				// ダメージを受ける処理
				life_--;
				Audio::GetInstance()->SoundPlayWave(damageSE_, 0.05f);
				emit.RandomTranslate({-3.5f,-3.5f}, {-0.2f,-0.2f}, {-4.0f,-4.0f});
				emit.pos = GetCenterPos();
				emit.BurstAnime();
			}
		}

	}


	// ボスと衝突したときは常にノックバック
	else if (collisionType == static_cast< uint32_t >(CollisionTypeIdDef::kBoss)){
		Boss* boss = static_cast< Boss* >(other);
		SphereCollider* bossCollider = static_cast< SphereCollider* >(other->GetCollider());
		SphereCollider* playerCollider = static_cast< SphereCollider* >(this->GetCollider());

		uint32_t serialNum = boss->GetSerialNumber();

		if (record_.CheckRecord(serialNum)){
			return;
		}

		// 履歴に登録
		record_.AddRecord(serialNum);

		Vector3 playerPos = playerCollider->GetPosition();
		Vector3 bossPos = bossCollider->GetPosition();

		if (!isKnockedBack_){

			// プレイヤーがボスと衝突した際に右方向（+X方向）に飛ばす
			const float kBossCollisionXVelocity = 0.2f;  // 右方向に飛ばす速度
			const float kBossCollisionYVelocity = 0.3f;  // 上方向に飛ばす速度

			// X方向の速度を設定して右に飛ばす
			velocity_.x = kBossCollisionXVelocity;
			// Y方向の速度も設定してジャンプさせる
			velocity_.y = kBossCollisionYVelocity;

			// プレイヤーをボスから少し右に押し出す
			playerPos.x += 0.5f;  // 右方向に押し出す距離を調整
			playerCollider->SetPosition(playerPos);

			// ノックバック状態に設定
			isKnockedBack_ = true;
			knockbackTimer_ = 0.5f;  // ノックバックの継続時間を設定

			life_--;
			Audio::GetInstance()->SoundPlayWave(damageSE_, 0.05f);
			emit.RandomTranslate({-1.5f,-1.5f}, {0.0f,0.0f}, {-8.0f,-8.0f});
			emit.pos = GetCenterPos();
			emit.BurstAnime();
		}

	}

	// 障害物と衝突したとき
	else if (collisionType == static_cast< uint32_t >(CollisionTypeIdDef::kObstacle)){
		Obstacle* obstacle = static_cast< Obstacle* >(other);
		SphereCollider* obstacleCollider = static_cast< SphereCollider* >(other->GetCollider());
		SphereCollider* playerCollider = static_cast< SphereCollider* >(this->GetCollider());

		uint32_t serialNum = obstacle->GetSerialNumber();

		if (record_.CheckRecord(serialNum)){
			return;
		}

		// 履歴に登録
		record_.AddRecord(serialNum);

		if (!isKnockedBack_){
			Vector3 playerPos = playerCollider->GetPosition();
			Vector3 obstaclePos = obstacleCollider->GetPosition();

			// プレイヤーと障害物のX座標の差を計算
			float xDifference = playerPos.x - obstaclePos.x;

			// ノックバックの方向を決定（右から衝突：右に、左から衝突：左に飛ばす）
			knockbackDirection_ = (xDifference > 0) ? 1.0f : -1.0f;

			const float kObstacleCollisionXVelocity = 0.2f * knockbackDirection_;  // 水平方向の速度
			const float kObstacleCollisionYVelocity = 0.3f;  // 上方向の速度

			// X方向の速度を設定
			velocity_.x = kObstacleCollisionXVelocity;
			// Y方向の速度も設定
			velocity_.y = kObstacleCollisionYVelocity;

			// プレイヤーを障害物から少し押し出す（右方向または左方向）
			playerPos.x += 0.5f * knockbackDirection_;
			playerCollider->SetPosition(playerPos);

			// ノックバック状態に設定
			isKnockedBack_ = true;
			knockbackTimer_ = 0.5f;  // ノックバックの継続時間を設定

			life_--;
			Audio::GetInstance()->SoundPlayWave(damageSE_, 0.05f);
			emit.RandomTranslate({-3.5f,-3.5f}, {-0.2f,-0.2f}, {-4.0f,-4.0f});
			emit.pos = GetCenterPos();
			emit.BurstAnime();

		}
	}

	if (life_ == 0){
		SceneManager::GetInstance()->SetGameOver(true);
		isGameover = true;
	}
}

bool Player::GameOverUpdate(){

	if (gameoverTime >= 0.0f){

		gameoverTime -= FPSKeeper::DeltaTime();


		if (130.0f <= gameoverTime && 128.0f <= gameoverTime){
			emit.RandomTranslate({-5.5f,1.5f}, {-1.6f,1.6f}, {-4.0f,-4.0f});
			emit.pos = GetCenterPos();
			emit.BurstAnime();
			CameraManager::GetInstance()->GetCamera()->SetShakeTime(40.0f);
		}
		if (80.0f <= gameoverTime && 78.0f <= gameoverTime){
			emit.RandomTranslate({-5.5f,1.5f}, {-1.6f,1.6f}, {-4.0f,-4.0f});
			emit.pos = GetCenterPos();
			emit.BurstAnime();
			CameraManager::GetInstance()->GetCamera()->SetShakeTime(40.0f);
		}if (30.0f <= gameoverTime && 28.0f <= gameoverTime){
			emit.RandomTranslate({-5.5f,1.5f}, {-1.6f,1.6f}, {-4.0f,-4.0f});
			emit.pos = GetCenterPos();
			emit.BurstAnime();
			CameraManager::GetInstance()->GetCamera()->SetShakeTime(40.0f);
		}
	} else{
		return false;
	}

	return true;
}

void Player::DrawUi(){
	// 現在のライフ数に基づいてスプライトを描画
	for (size_t i = 0; i < 4; i++){
		if (i < life_){  // 現在のライフ数以下のインデックスのスプライトのみ描画
			lifeSprite_[i]->Draw();
		}
	}
}
