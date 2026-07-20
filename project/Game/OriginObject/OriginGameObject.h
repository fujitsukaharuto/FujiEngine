#pragma once
#include <memory>
// json は宣言とポインタにしか使わないので前方宣言で足りる
#include <json_fwd.hpp>

#include "Engine/Model/Object3d.h"
#include "Engine/Model/AnimationData/AnimationModel.h"
#include "Engine/DX/FPSKeeper.h"
#include "Engine/Camera/CameraManager.h"
#include "Engine/Input/Input.h"

/// <summary>
/// ゲームオブジェクトの基底クラス
/// </summary>
class OriginGameObject {
public:
	OriginGameObject();
	/// <remarks>modelDataJson_ が不完全型の unique_ptr なので定義は .cpp に置く</remarks>
	virtual ~OriginGameObject();

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

protected:

	std::unique_ptr<Graphics::Object3d> model_;
	std::unique_ptr<Graphics::AnimationModel> animeModel_;

	/// <summary>Jsonから生成する際の元データ。json.hpp をヘッダから隔離するため実体は持たない</summary>
	std::unique_ptr<nlohmann::json> modelDataJson_;

private:

};
