#pragma once
#include <memory>
#include <cassert>
// json は宣言とポインタにしか使わないので前方宣言で足りる
#include <json_fwd.hpp>

#include "Engine/Model/Object3d.h"
#include "Engine/Model/AnimationData/AnimationModel.h"
#include "Engine/Model/Base/RenderObject.h"
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
	/// <remarks>生成していない側は nullptr を返す</remarks>
	Graphics::Object3d* GetModel() { return model_.get(); }
	Graphics::AnimationModel* GetAnimeModel() { return animeModel_.get(); }

	/// <summary>生成済みの描画オブジェクトを基底(RenderObject)として返す</summary>
	/// <remarks>model_ と animeModel_ は同時に生成しない設計。生成済みの方を返し、未生成なら nullptr</remarks>
	Graphics::RenderObject* GetRenderObject() {
		if (model_) { return model_.get(); }
		return animeModel_.get();
	}
	const Graphics::RenderObject* GetRenderObject() const {
		if (model_) { return model_.get(); }
		return animeModel_.get();
	}

	// Object3d / AnimationModel のどちらで生成していても Transform・ワールド座標を取れる
	Math::Trans& GetTrans() { Graphics::RenderObject* o = GetRenderObject(); assert(o && "このオブジェクトは描画モデルを生成していない"); return o->GetTransform(); }
	Math::Vector3 GetWorldPos()const { const Graphics::RenderObject* o = GetRenderObject(); assert(o && "このオブジェクトは描画モデルを生成していない"); return o->GetWorldPos(); }

protected:

	/// <summary>model_ を必要になった時点で生成する</summary>
	/// <remarks>model_ を直接操作したい派生クラスは、先にこれを通すこと</remarks>
	Graphics::Object3d* EnsureModel();
	/// <summary>animeModel_ を必要になった時点で生成する</summary>
	Graphics::AnimationModel* EnsureAnimeModel();

protected:

	std::unique_ptr<Graphics::Object3d> model_;
	std::unique_ptr<Graphics::AnimationModel> animeModel_;

	/// <summary>Jsonから生成する際の元データ。json.hpp をヘッダから隔離するため実体は持たない</summary>
	std::unique_ptr<nlohmann::json> modelDataJson_;

private:

};
