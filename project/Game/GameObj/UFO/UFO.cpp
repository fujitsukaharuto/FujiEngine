#include "UFO.h"
///behavior
#include"Behavior/UFORoot.h"
#include"State/UFOMissilePop.h"
#include"State/UFOPopWait.h"
#include"Behavior/UFODamage.h"
#include"GameObj/Enemy/Behavior/EnemyBlowingWeak.h"
//obj
#include"GameObj/Enemy/EnemyManager.h"
#include"GameObj/Enemy/BaseEnemy.h"

#include "PointLightManager.h"

///* imgui
#include<imgui.h> 
#include"ImGuiManager.h"

float UFO::InitY_ = 44.6f;

UFO::UFO() {}

/// ===================================================
///  初期化
/// ===================================================
void UFO::Initialize() {

	OriginGameObject::Initialize();
	OriginGameObject::CreateModel("UFO.obj");

	ufoLight_ = std::make_unique<Object3d>();
	ufoLight_->Create("ufo_light.obj");
	ufoLight_->SetParent(model_.get());
	ufoLight_->transform.translate.x = -1.0f;
	ufoLight_->transform.scale = paramater_.initLightScale;
	ufoLight_->SetColor(ufoLightColor_);

	PointLightManager::GetInstance()->GetSpotLight(0)->spotLightData_->position = model_->transform.translate;

	hpSprite_ = std::make_unique<Sprite>();
	hpSprite_->Load("white2x2.png");
	hpSprite_->SetColor({ 0.8f,0.0f ,0.0f ,1.0f });
	hpSprite_->SetAnchor({ 0.0f,0.0f });
	hpSprite_->SetSize({ 500.0f,16.0f });
	hpSprite_->SetPos({ 150.0f,38.0f,0.0f });

	hpMaxSprite_ = std::make_unique<Sprite>();
	hpMaxSprite_->Load("white2x2.png");
	hpMaxSprite_->SetColor({ 0.023f,0.023f ,0.023f ,1.0f });
	hpMaxSprite_->SetAnchor({ 0.5f,0.5f });
	hpMaxSprite_->SetSize({ 520.0f,36.0f });
	hpMaxSprite_->SetPos({ 400.0f,50.0f,0.0f });



	///* グローバルパラメータ
	globalParameter_ = GlobalVariables::GetInstance();
	globalParameter_->CreateGroup(groupName_, false);
	AddParmGroup();
	ApplyGlobalParameter();

	moveRestrectionPos_ = 25.0f;
	moveDirection = 1.0f;
	MaxHp_ = 30.0f;
	hp_ = MaxHp_;
	

	// collider
	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetTag("UFO");
	SetCollisionSize(paramater_.collisionSize_);
	collider_->SetParent(model_.get());
	collider_->InfoUpdate();

	/// 通常モードから
	ChangeBehavior(std::make_unique<UFORoot>(this));
	ChangeState(std::make_unique<UFOPopWait>(this));
}

/// ===================================================
///  更新処理
/// ===================================================
void UFO::Update() {
	popPos_ = model_->GetWorldPos();
	/// ダメージエフェクト
	DamageRendition();
	/// 振る舞い処理
	behavior_->Update();
	state_->Update();
	///
	collider_->InfoUpdate();
	

	hpSize_ = hp_ / MaxHp_ * hpMaxSize_;
	if (hpSize_ < 0.0f) {
		hpSize_ = 0.0f;
	}
	hpSprite_->SetSize({ hpSize_,25.0f });

	PointLightManager::GetInstance()->GetSpotLight(0)->spotLightData_->position = model_->transform.translate;
	PointLightManager::GetInstance()->GetSpotLight(0)->spotLightData_->position.z = 0.0f;
}

/// ===================================================
///  描画
/// ===================================================
void UFO::Draw(Material* material) {

	OriginGameObject::Draw(material);
	collider_->DrawCollider();
}

void UFO::UFOLightDraw() {
	ufoLight_->Draw();
}

void UFO::SpriteDraw() {
	hpMaxSprite_->Draw();
	hpSprite_->Draw();
}


void UFO::EnemySpawn() {
	/*pEnemyManager_->SpawnEnemy("NormalEnemy", popPos_);*/
}

/// ===================================================
///  ダメージ演出
/// ===================================================
void UFO::DamageRendition() {
	
}


///==========================================================
/// ダメージ受ける
///==========================================================
void UFO::TakeDamageForPar(const float& par) {

	//割合によるインクる面とする値を決める
	float decrementSize = MaxHp_ * (par / 100.0f);;
	// HP減少
	hp_ -= decrementSize;

	//HPが0以下にならないように
	if (hp_ <= 0) {
		// 死亡処理
		isDeath_ = true;
	}
}

void UFO::Move() {
	model_->transform.translate.x += 3.0f * moveDirection * FPSKeeper::DeltaTimeRate();
	if (std::abs(model_->transform.translate.x) > moveRestrectionPos_) {
		moveDirection = -moveDirection;
		model_->transform.translate.x = Clamp(model_->transform.translate.x, -moveRestrectionPos_, moveRestrectionPos_);
	}
}


///=========================================================
/// パラメータ調整
///==========================================================
void UFO::AdjustParm() {
	SetValues();
#ifdef _DEBUG
	if (ImGui::CollapsingHeader(groupName_.c_str())) {
		ImGui::PushID(groupName_.c_str());
		ImGuiManager::GetInstance()->SetFontJapanese();/// 日本語
	
		ImGui::DragFloat3("位置", &model_->transform.translate.x, 0.1f);
		ImGui::DragFloat3("当たり判定サイズ", &paramater_.collisionSize_.x, 0.1f);

		ImGui::DragFloat("ダメージ中の時間(s)", &paramater_.damageTime_, 0.01f);
		ImGui::DragFloat("ヒットストップをするダメージ量(%)", &paramater_.hitStopDamage_, 0.01f);
		ImGui::DragFloat("ヒットストップ時間(%)", &paramater_.hitStopTime_, 0.01f);

		ImGui::DragFloat3("通常ライトサイズ", &paramater_.initLightScale.x, 0.01f);
		ImGui::DragFloat3("ミサイル出す時のライトサイズ", &paramater_.lightScaleUnderPop.x, 0.01f);

		ImGui::SeparatorText("いらないかも");
		ImGui::DragFloat("ダメージの吹っ飛び距離", &paramater_.dagameDistance_, 0.01f);
			
		/// セーブとロード
		globalParameter_->ParmSaveForImGui(groupName_);
		ParmLoadForImGui();

		ImGui::SeparatorText("セーブできないデバッグ用");
		ImGui::DragFloat("体力", &hp_, 0.01f);
		ImGui::DragFloat3("size", &model_->transform.scale.x, 0.1f);

		ImGuiManager::GetInstance()->UnSetFont();
		ImGui::PopID();
	}


	ImGui::Begin("lightSetting");
	ImGui::DragFloat3("lightSize", &ufoLight_->transform.scale.x, 0.1f);
	ImGui::DragFloat3("lightPos", &ufoLight_->transform.translate.x, 0.1f);
	ImGui::ColorEdit4("ufoLightColor", &ufoLightColor_.x);
	ufoLight_->SetColor(ufoLightColor_);
	ImGui::End();

	PointLightManager::GetInstance()->GetSpotLight(0)->Debug();

#endif // _DEBUG
}
///=================================================================================
/// ロード
///=================================================================================
void UFO::ParmLoadForImGui() {

	// ロードボタン
	if (ImGui::Button(std::format("Load {}", groupName_).c_str())) {

		globalParameter_->LoadFile(groupName_);
		// セーブ完了メッセージ
		ImGui::Text("Load Successful: %s", groupName_.c_str());
		ApplyGlobalParameter();
	}
}

///=================================================================================
///パラメータをグループに追加
///=================================================================================
void UFO::AddParmGroup() {

	globalParameter_->AddItem(groupName_, "Translate", model_->transform.translate);
	globalParameter_->AddItem(groupName_, "DamageTime", paramater_.damageTime_);
	globalParameter_->AddItem(groupName_, "DamageDistance", paramater_.dagameDistance_);
	globalParameter_->AddItem(groupName_, "collisionSize_", paramater_.collisionSize_);
	globalParameter_->AddItem(groupName_, "hitStopDamage_", paramater_.hitStopDamage_);
	globalParameter_->AddItem(groupName_, "hitStopTime_", paramater_.hitStopTime_);
	globalParameter_->AddItem(groupName_, "rootLightScale", paramater_.initLightScale);
	globalParameter_->AddItem(groupName_, "lightScaleUnderPop", paramater_.lightScaleUnderPop);
}

///=================================================================================
///パラメータをグループに追加
///=================================================================================
void UFO::SetValues() {
	
	globalParameter_->SetValue(groupName_, "Translate", model_->transform.translate);
	globalParameter_->SetValue(groupName_, "collisionSize_", paramater_.collisionSize_);
	globalParameter_->SetValue(groupName_, "DamageTime", paramater_.damageTime_);
	globalParameter_->SetValue(groupName_, "DamageDistance", paramater_.dagameDistance_);
	globalParameter_->SetValue(groupName_, "hitStopDamage_", paramater_.hitStopDamage_);
	globalParameter_->SetValue(groupName_, "hitStopTime_", paramater_.hitStopTime_);
	globalParameter_->SetValue(groupName_, "rootLightScale", paramater_.initLightScale);
	globalParameter_->SetValue(groupName_, "lightScaleUnderPop", paramater_.lightScaleUnderPop);
}

///=====================================================
///  ImGuiからパラメータを得る
///===================================================== 
void UFO::ApplyGlobalParameter() {
	model_->transform.translate = globalParameter_->GetValue<Vector3>(groupName_, "Translate");
	paramater_.collisionSize_ = globalParameter_->GetValue<Vector3>(groupName_, "collisionSize_");
	paramater_.damageTime_ = globalParameter_->GetValue<float>(groupName_, "DamageTime");
	paramater_.dagameDistance_ = globalParameter_->GetValue<float>(groupName_, "DamageDistance");
	paramater_.hitStopDamage_ = globalParameter_->GetValue<float>(groupName_, "hitStopDamage_");
	paramater_.hitStopTime_ = globalParameter_->GetValue<float>(groupName_, "hitStopTime_");
	paramater_.initLightScale = globalParameter_->GetValue<Vector3>(groupName_, "rootLightScale");
	paramater_.lightScaleUnderPop = globalParameter_->GetValue<Vector3>(groupName_, "lightScaleUnderPop");
}
///=========================================================
/// Class Set
///==========================================================

void UFO::SetEnemyManager(EnemyManager* enemymanager) {
	pEnemyManager_ = enemymanager;
}

void UFO::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {

	/// 弱い吹っ飛びをくらってる
	if (other.tag == "BlowingWeakEnemy") {

		// ダメージ量分受ける
		for (auto& enemy : pEnemyManager_->GetEnemies()) {
			if (dynamic_cast<EnemyBlowingWeak*>(enemy->GetBehavior())) {
				takeDamageValue_ = enemy->GetSumWeakAttackValue();
				TakeDamageForPar(takeDamageValue_);
			}
		}
		hitPosition_ = other.pos;

		// ダメージ演出
		if (dynamic_cast<UFODamage*>(behavior_.get()))return;
		ChangeBehavior(std::make_unique<UFODamage>(this));
		return;
	}

	/// つおい吹っ飛び
	if (other.tag == "BlowingStrongEnemy") {
		takeDamageValue_ = pEnemyManager_->GetParamater(static_cast<size_t>(BaseEnemy::Type::NORMAL)).strongAttackValue;
		//デカダメージ受ける
		TakeDamageForPar(takeDamageValue_);
		hitPosition_ = other.pos;

		// ダメージ演出
		if (dynamic_cast<UFODamage*>(behavior_.get()))return;
		ChangeBehavior(std::make_unique<UFODamage>(this));
		return;
	}


}

void UFO::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {

}

void UFO::SetColor(const Vector4& color) {
	model_->SetColor(color);
}

void  UFO::SetCollisionSize(const Vector3& size) {
	collider_->SetWidth(size.x);
	collider_->SetHeight(size.y);
	collider_->SetDepth(size.z);
}

///=========================================================
/// 振る舞い切り替え
///==========================================================
void UFO::ChangeBehavior(std::unique_ptr<BaseUFOBehavior>behavior) {
	//引数で受け取った状態を次の状態としてセット
	behavior_ = std::move(behavior);
}

void UFO::ChangeState(std::unique_ptr<BaseUFOState>behavior) {
	state_ = std::move(behavior);
}

void UFO::ChangePopBehavior() {
	ChangeState(std::make_unique<UFOMissilePop>(this));
}