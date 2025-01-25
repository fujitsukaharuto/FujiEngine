#pragma once
#include <memory>
#include "Model/Object3d.h"


class OriginGameObject {
public:
	OriginGameObject();
	virtual ~OriginGameObject() = default;

	virtual void Initialize();
	virtual void Update();
	virtual void Draw(Material* mate = nullptr);

	virtual void Debug();


public:

	void CreateModel(const std::string& name);
	void SetModel(const std::string& name);
	Object3d* GetModel() { return model_.get(); }
	Trans& GetTrans() { return model_->transform; }

	void SetWorldPosition(Vector3 pos) { model_->transform.translate = pos; }
	void SetWorldPositionY(float pos) { model_->transform.translate.y = pos; }
	void SetWorldPositionX(float pos) { model_->transform.translate.x = pos; }
	void SetWorldPositionZ(float pos) { model_->transform.translate.z = pos; }
	void AddPosition(Vector3 pos) { model_->transform.translate += pos; }
	void SetRotation(Vector3 rotate) { model_->transform.rotate = rotate; }
	void SetRotationY(float rotate) { model_->transform.rotate.y = rotate; }
	void SetRotationX(float rotate) { model_->transform.rotate.x = rotate; }
	void SetRotationZ(float rotate) { model_->transform.rotate.z = rotate; }
	void SetScale(Vector3 scale) { model_->transform.scale = scale; }

protected:

	std::unique_ptr<Object3d> model_;

};
