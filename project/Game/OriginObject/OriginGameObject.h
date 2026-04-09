#pragma once
#include <memory>
#include <json.hpp>

#include "Model/Object3d.h"
#include "Model/AnimationData/AnimationModel.h"
#include "Engine/DX/FPSKeeper.h"
#include "Camera/CameraManager.h"
#include "Input/Input.h"
#include "Engine/ImGuiManager/ImGuiManager.h"

/// <summary>
/// ゲームオブジェクトの基底クラス
/// </summary>
class OriginGameObject {
public:
	OriginGameObject();
	virtual ~OriginGameObject() = default;

	virtual void Initialize();
	virtual void Update();
	virtual void Draw(bool is = false);
	virtual void DebugGUI();

#ifdef _DEBUGMODE
	virtual void Debug();
#endif // _DEBUG

public:

	/// <summary>値比較</summary>
	float ComparNum(float a, float b);
	/// <summary>モデル作成</summary>
	void CreateModel(const std::string& name);
	/// <summary>アニメーションモデル作成</summary>
	void CreateAnimeModel(const std::string& name);
	/// <summary>Jsonから作成</summary>
	void CreateFromJson(const std::string& name);
	void CreateFromJson();

	//========================================================================*/
	//* Setter
	void SetModel(const std::string& name);
	void SetAnimeModel(const std::string& name);
	void SetModelDataJson(const nlohmann::json& jsonData);

	//========================================================================*/
	//* Getter
	Graphics::Object3d* GetModel() { return model_.get(); }
	Graphics::AnimationModel* GetAnimeModel() { return animeModel_.get(); }
	Math::Trans& GetTrans() { return model_->GetTransform(); }
	Math::Trans& GetAnimeTrans() { return animeModel_->GetTransform(); }
	Math::Vector3 GetWorldPos()const { return model_->GetWorldPos(); }
	Math::Vector3 GetAnimeWorldPos()const { return animeModel_->GetWorldPos(); }

	void CreatePropertyCommand(int type);

protected:

	std::unique_ptr<Graphics::Object3d> model_;
	std::unique_ptr<Graphics::AnimationModel> animeModel_;


	Math::Vector3 prevPos_;
	Math::Vector3 prevRotate_;
	Math::Vector3 prevScale_;
	int gizmoType_ = 0;
	float IsUsingGizmo_ = false;

	nlohmann::json modelDataJson_;

private:

};
