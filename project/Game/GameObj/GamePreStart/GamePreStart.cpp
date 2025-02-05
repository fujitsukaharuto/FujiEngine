#include"GamePreStart.h"
#include"GameObj/UFO/UFO.h"
#include"GameObj/SkyDome/SkyDome.h"
#include"GameObj/BackGround/BackGround.h"
#include"FPSKeeper.h"
#include"ImGuiManager.h"
#include"Input/Input.h"
#include<imgui.h>

void GamePreStart::Init() {
	aimSprite_ = std::make_unique<Sprite>();
	aimSprite_->Load("GameTexture/StartUI.png");
	aimSprite_->SetAnchor({ 0.5f,0.5f });

	kikAimSprite_ = std::make_unique<Sprite>();
	kikAimSprite_->Load("GameTexture/StartUI_Kick.png");
	kikAimSprite_->SetAnchor({ 0.5f,0.5f });

	blackSprite_ = std::make_unique<Sprite>();
	blackSprite_->Load("white2x2.png");
	blackSprite_->SetColor({ 0.0f,0.0f,0.0f,0.5f });
	blackSprite_->SetSize({ 1280.0f,950.0f });
	blackSprite_->SetAnchor({ 0.0f,0.0f });

	///* グローバルパラメータ
	globalParameter_ = GlobalVariables::GetInstance();
	globalParameter_->CreateGroup(groupName_, false);
	AddParmGroup();
	ApplyGlobalParameter();

	
	aimPosX = params_.aimStartPos;

	///pos
	aimSprite_->SetPos(Vector3(params_.aimStartPos, 110.0f, 0));
	kikAimSprite_->SetPos(Vector3(params_.kikAimPos.x, params_.kikAimPos.y, 0));
	kikAimSprite_->SetSize(Vector2(0, 0));

	step_ = Step::WAIT;
}

//void GameOver::SetStepStart() {
//	step_ = Step::FADEIN;
//}

void GamePreStart::Update() {

	aimSprite_->SetPos(Vector3(aimPosX, 240.0f, 0));
	kikAimSprite_->SetPos(Vector3(params_.kikAimPos.x, params_.kikAimPos.y, 0));

	switch (step_)
	{
		///-----------------------------------------------------------------
		/// 最初の待機
		///-----------------------------------------------------------------
	case GamePreStart::Step::WAIT:
		times_[static_cast<size_t>(Step::WAIT)] += FPSKeeper::NormalDeltaTime();
		if (times_[static_cast<size_t>(Step::WAIT)] < params_.kWaitTime)break;
		times_[static_cast<size_t>(Step::WAIT)] = params_.kWaitTime;
		step_ = Step::GOUPGROUND;
		break;

		///-----------------------------------------------------------------
		/// 上に上がる
		///-----------------------------------------------------------------
	case GamePreStart::Step::GOUPGROUND:
		times_[static_cast<size_t>(Step::GOUPGROUND)] += FPSKeeper::NormalDeltaTime();
		/// 出現
		pBackGround_->Scrool(times_[static_cast<size_t>(Step::GOUPGROUND)], params_.goUpTimeMax);

		if (times_[static_cast<size_t>(Step::GOUPGROUND)] < params_.goUpTimeMax)break;
		times_[static_cast<size_t>(Step::GOUPGROUND)] = params_.goUpTimeMax;
		step_ = Step::APEARUFO;
		break;
		///-----------------------------------------------------------------
		/// 上に上がる
		///-----------------------------------------------------------------
	case GamePreStart::Step::APEARUFO:
		times_[static_cast<size_t>(Step::APEARUFO)] += FPSKeeper::NormalDeltaTime();
		/// 出現
		pUFO_->Apear(times_[static_cast<size_t>(Step::APEARUFO)], params_.apearUFOMax);

		if (times_[static_cast<size_t>(Step::APEARUFO)] < params_.apearUFOMax)break;
		times_[static_cast<size_t>(Step::APEARUFO)] = params_.apearUFOMax;
		step_ = Step::AIMWAIT;
		break;
		///-----------------------------------------------------------------
		/// 最初の待機
		///-----------------------------------------------------------------
	case GamePreStart::Step::AIMWAIT:
		times_[static_cast<size_t>(Step::AIMWAIT)] += FPSKeeper::NormalDeltaTime();
		if (times_[static_cast<size_t>(Step::AIMWAIT)] < params_.aimKWaitTime)break;
		times_[static_cast<size_t>(Step::AIMWAIT)] = params_.aimKWaitTime;
		step_ = Step::AIMOPEN;
		break;
		///-----------------------------------------------------------------
		/// 最初の待機
		///-----------------------------------------------------------------
	case GamePreStart::Step::AIMOPEN:
		times_[static_cast<size_t>(Step::AIMOPEN)] += FPSKeeper::NormalDeltaTime();
		aimPosX = EaseOutCubic(params_.aimStartPos, params_.aimEndPos, times_[static_cast<size_t>(Step::AIMOPEN)], params_.aimEaseMax);

		if (times_[static_cast<size_t>(Step::AIMOPEN)] < params_.aimEaseMax)break;
		times_[static_cast<size_t>(Step::AIMOPEN)] = params_.aimEaseMax;
		aimPosX = params_.aimEndPos;
		step_ = Step::AIMKIKOPEN;

		break;
		///-----------------------------------------------------------------
		/// キックエイム
		///-----------------------------------------------------------------
	case GamePreStart::Step::AIMKIKOPEN:
		//タイム
		times_[static_cast<size_t>(Step::AIMKIKOPEN)] += FPSKeeper::NormalDeltaTime();
		params_.kikAimScale = EaseOutCubic(Vector2(0, 0), Vector2(1, 1), times_[static_cast<size_t>(Step::AIMKIKOPEN)], params_.kikAimEaseMax);

		///適応
		kikAimSprite_->SetSize(Vector2(kikTextureSize_.x * params_.kikAimScale.x, kikTextureSize_.y * params_.kikAimScale.y));

		if (times_[static_cast<size_t>(Step::AIMKIKOPEN)] < params_.kikAimEaseMax)break;
		times_[static_cast<size_t>(Step::AIMKIKOPEN)] = params_.kikAimEaseMax;
		kikAimSprite_->SetSize(kikTextureSize_);
		step_ = Step::CLOSEWAIT;

		break;
		///-----------------------------------------------------------------
		///閉め待機
		///-----------------------------------------------------------------
	case GamePreStart::Step::CLOSEWAIT:
		times_[static_cast<size_t>(Step::CLOSEWAIT)] += FPSKeeper::NormalDeltaTime();
		if (times_[static_cast<size_t>(Step::CLOSEWAIT)] < params_.closekWaitTime)break;
		times_[static_cast<size_t>(Step::CLOSEWAIT)] = params_.closekWaitTime;
		step_ = Step::AIMCLOSE;
		break;
		///-----------------------------------------------------------------
		///閉め
		///-----------------------------------------------------------------
	case GamePreStart::Step::AIMCLOSE:
		// タイム
		times_[static_cast<size_t>(Step::AIMCLOSE)] += FPSKeeper::NormalDeltaTime();
		closeScale_ = EaseInBack(1.0f, 0.0f, times_[static_cast<size_t>(Step::AIMCLOSE)], params_.aimCloseEaseMax);

		///適応
		kikAimSprite_->SetSize(Vector2(kikTextureSize_.x * closeScale_, kikTextureSize_.y * closeScale_));
		aimSprite_->SetSize(Vector2(aimTextureSize_.x * closeScale_, aimTextureSize_.y * closeScale_));

		/// 終了
		if (times_[static_cast<size_t>(Step::AIMCLOSE)] < params_.aimCloseEaseMax)break;
		times_[static_cast<size_t>(Step::AIMCLOSE)] = params_.aimEaseMax;
		kikAimSprite_->SetSize(Vector2(0, 0));
		aimSprite_->SetSize(Vector2(0, 0));
		//エンド
		isEnd_ = true;
		step_ = Step::END;
		break;
		///-----------------------------------------------------------------
		/// 終わり
		///-----------------------------------------------------------------
	case GamePreStart::Step::END:

		break;
	default:
		break;
	}
}

void GamePreStart::Draw() {
	aimSprite_->Draw();
	kikAimSprite_->Draw();
}

void  GamePreStart::BlockSpriteDraw() {
	if (step_ == Step::AIMOPEN ||
		step_ == Step::AIMKIKOPEN ||
		step_ == Step::CLOSEWAIT ||
		step_ == Step::AIMCLOSE) {
		blackSprite_->Draw();
	}
}

void GamePreStart::Debug() {
	//ImGui::DragFloat3("selectPos_", &selectPos_.x, 0.1f);
}


void GamePreStart::OffsetMove() {

}

void GamePreStart::SetSkyDome(SkyDome* skydome) {
	pSkyDome_ = skydome;
}
void GamePreStart::SetUFO(UFO* ufo) {
	pUFO_ = ufo;
}
void GamePreStart::SetBackGround(BackGround* back) {
	pBackGround_ = back;
}


///=================================================================================
/// ロード
///=================================================================================
void GamePreStart::ParmLoadForImGui() {

	// ロードボタン
	if (ImGui::Button(std::format("Load {}", groupName_).c_str())) {

		globalParameter_->LoadFile(groupName_);
		// セーブ完了メッセージ
		ImGui::Text("Load Successful: %s", groupName_.c_str());
		ApplyGlobalParameter();
	}
}

//////=================================================================================
///パラメータをグループに追加
///=================================================================================
void GamePreStart::AddParmGroup() {
	globalParameter_->AddItem(groupName_, "kWaitTime", params_.kWaitTime);
	globalParameter_->AddItem(groupName_, "goUpTimeMax", params_.goUpTimeMax);
	globalParameter_->AddItem(groupName_, "apearUFOMax", params_.apearUFOMax);
	globalParameter_->AddItem(groupName_, "aimKWaitTime", params_.aimKWaitTime);
	globalParameter_->AddItem(groupName_, "aimEaseMax", params_.aimEaseMax);
	globalParameter_->AddItem(groupName_, "aimStartPos", params_.aimStartPos);
	globalParameter_->AddItem(groupName_, "aimEndPos", params_.aimEndPos);
	globalParameter_->AddItem(groupName_, "closekWaitTime", params_.closekWaitTime);
	globalParameter_->AddItem(groupName_, "aimCloseEndPos", params_.aimCloseEndPos);
	globalParameter_->AddItem(groupName_, "aimCloseEaseMax", params_.aimCloseEaseMax);
	globalParameter_->AddItem(groupName_, "kikAimEaseMax", params_.kikAimEaseMax);
	globalParameter_->AddItem(groupName_, "kikAimPos", params_.kikAimPos);
}

///=================================================================================
///パラメータをグループに追加
///=================================================================================
void GamePreStart::SetValues() {
	globalParameter_->SetValue(groupName_, "kWaitTime", params_.kWaitTime);
	globalParameter_->SetValue(groupName_, "goUpTimeMax", params_.goUpTimeMax);
	globalParameter_->SetValue(groupName_, "apearUFOMax", params_.apearUFOMax);
	globalParameter_->SetValue(groupName_, "aimKWaitTime", params_.aimKWaitTime);
	globalParameter_->SetValue(groupName_, "aimEaseMax", params_.aimEaseMax);
	globalParameter_->SetValue(groupName_, "aimStartPos", params_.aimStartPos);
	globalParameter_->SetValue(groupName_, "aimEndPos", params_.aimEndPos);
	globalParameter_->SetValue(groupName_, "closekWaitTime", params_.closekWaitTime);
	globalParameter_->SetValue(groupName_, "aimCloseEndPos", params_.aimCloseEndPos);
	globalParameter_->SetValue(groupName_, "aimCloseEaseMax", params_.aimCloseEaseMax);
	globalParameter_->SetValue(groupName_, "kikAimEaseMax", params_.kikAimEaseMax);
	globalParameter_->SetValue(groupName_, "kikAimPos", params_.kikAimPos);
}

///=====================================================
/// ImGuiからパラメータを得る
///===================================================== 
void GamePreStart::ApplyGlobalParameter() {
	params_.kWaitTime = globalParameter_->GetValue<float>(groupName_, "kWaitTime");
	params_.goUpTimeMax = globalParameter_->GetValue<float>(groupName_, "goUpTimeMax");
	params_.apearUFOMax = globalParameter_->GetValue<float>(groupName_, "apearUFOMax");
	params_.aimKWaitTime = globalParameter_->GetValue<float>(groupName_, "aimKWaitTime");
	params_.aimEaseMax = globalParameter_->GetValue<float>(groupName_, "aimEaseMax");
	params_.aimStartPos = globalParameter_->GetValue<float>(groupName_, "aimStartPos");
	params_.aimEndPos = globalParameter_->GetValue<float>(groupName_, "aimEndPos");
	params_.closekWaitTime = globalParameter_->GetValue<float>(groupName_, "closekWaitTime");
	params_.aimCloseEndPos = globalParameter_->GetValue<float>(groupName_, "aimCloseEndPos");
	params_.aimCloseEaseMax = globalParameter_->GetValue<float>(groupName_, "aimCloseEaseMax");
	params_.kikAimEaseMax = globalParameter_->GetValue<float>(groupName_, "kikAimEaseMax");
	params_.kikAimPos = globalParameter_->GetValue<Vector2>(groupName_, "kikAimPos");
}

///=========================================================
/// パラメータ調整
///==========================================================
void GamePreStart::AdjustParm() {
	SetValues();
#ifdef _DEBUG
	if (ImGui::CollapsingHeader(groupName_.c_str())) {
		ImGui::PushID(groupName_.c_str());
		ImGuiManager::GetInstance()->SetFontJapanese();/// 日本語

		// パラメータ表示・調整
		ImGui::DragFloat("待機時間", &params_.kWaitTime, 0.01f);
		ImGui::DragFloat("上昇最大時間", &params_.goUpTimeMax, 0.01f);
		ImGui::DragFloat("UFO出現最大時間", &params_.apearUFOMax, 0.01f);
		ImGui::DragFloat("狙い待機時間", &params_.aimKWaitTime, 0.01f);
		ImGui::DragFloat("狙いイージング最大値", &params_.aimEaseMax, 0.01f);
		ImGui::DragFloat("狙い開始位置", &params_.aimStartPos, 0.5f);
		ImGui::DragFloat("狙い終了位置", &params_.aimEndPos, 0.5f);
		ImGui::DragFloat("閉じる待機時間", &params_.closekWaitTime, 0.01f);
		ImGui::DragFloat("キック狙いイージング", &params_.kikAimEaseMax, 0.01f);
		ImGui::DragFloat2("キック狙い位置", &params_.kikAimPos.x, 0.5f);
		ImGui::DragFloat("狙い閉じるイージング最大値", &params_.aimCloseEaseMax, 0.01f);
	
		if (Input::GetInstance()->TriggerKey(DIK_R)) {
			step_ = Step::WAIT;
			for (size_t i = 0; i < times_.size(); i++) {
				times_[i] = 0.0f;
			}
		}

		/// セーブとロード
		globalParameter_->ParmSaveForImGui(groupName_);
		ParmLoadForImGui();

		ImGuiManager::GetInstance()->UnSetFont();
		ImGui::PopID();
	}

#endif // _DEBUG
}