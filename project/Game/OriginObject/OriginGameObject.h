#pragma once
#include <memory>
#include <json.hpp>

#include "Model/Object3d.h"
#include "Model/AnimationData/AnimationModel.h"
#include "Engine/DX/FPSKeeper.h"
#include "Camera/CameraManager.h"
#include "Input/Input.h"
#include "Engine/ImGuiManager/ImGuiManager.h"

class OriginGameObject {
public:
	OriginGameObject();
	virtual ~OriginGameObject() = default;

	virtual void Initialize();
	virtual void Update();
	virtual void Draw(Material* mate = nullptr, bool is = false);
	virtual void DebugGUI();

#ifdef _DEBUG
	virtual void Debug();
#endif // _DEBUG

public:

	float ComparNum(float a, float b);
	void CreateModel(const std::string& name);
	void CreateAnimModel(const std::string& name);
	void CreateFromJson(const std::string& name);
	void CreateFromJson();
	void SetModel(const std::string& name);
	void SetAnimModel(const std::string& name);
	void SetModelDataJson(const nlohmann::json& jsonData);
	Object3d* GetModel() { return model_.get(); }
	AnimationModel* GetAnimModel() { return animModel_.get(); }
	Trans& GetTrans() { return model_->transform; }
	Trans& GetAnimTrans() { return animModel_->transform; }
	Vector3 GetWorldPos()const { return model_->GetWorldPos(); }
	Vector3 GetAnimWorldPos()const { return animModel_->GetWorldPos(); }

	void CreatePropertyCommand(int type);

protected:

	std::unique_ptr<Object3d> model_;
	std::unique_ptr<AnimationModel> animModel_;


	Vector3 prevPos_;
	Vector3 prevRotate_;
	Vector3 prevScale_;
	int guizmoType_ = 0;
	float IsUsingGuizmo_ = false;

	nlohmann::json modelDataJson_;

private:

};
