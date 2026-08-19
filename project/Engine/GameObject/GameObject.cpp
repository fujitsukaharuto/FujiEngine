#include "GameObject.h"
#include <json.hpp>
#include "Engine/Core/Serialize/JsonSerializer.h"

using namespace Collision;

// ★名前空間とクラス名が同名なので using namespace GameObject; は書けない(曖昧になる)。
// 定義は名前空間ブロックで囲む
namespace GameObject {

	using namespace Core;
	using namespace Graphics;
	using namespace Math;
	using namespace Editor;


	GameObject::GameObject() : modelDataJson_(std::make_unique<nlohmann::json>()) {
	}

	GameObject::~GameObject() = default;

	void GameObject::Initialize() {
		// 描画オブジェクトはここで両方作らない。
		// CreateModel系が呼ばれたときに必要な方だけ生成する
		// (両方作ると、片方しか使わないオブジェクトが空のObject3d/AnimationModelを抱え続け、
		//  GetTransが「一度もCreateされていない方」のTransformを静かに返す事故も起きる)
	}

	Graphics::Object3d* GameObject::EnsureModel() {
		if (!model_) {
			model_ = std::make_unique<Object3d>();
		}
		return model_.get();
	}

	Graphics::AnimationModel* GameObject::EnsureAnimeModel() {
		if (!animeModel_) {
			animeModel_ = std::make_unique<AnimationModel>();
		}
		return animeModel_.get();
	}

	void GameObject::Update() {
	}

	void GameObject::Draw(bool is) {
		// 子ビジュアルを先に描く。影のような非additiveの子は本体より前に描く必要があるため
		// (additiveの子は順不同なので、この順で全ケースを満たせる)
		for (auto& r : renderers_) {
			if (r.visible) {
				r.object->Draw(is);
			}
		}
		if (RenderObject* o = GetRenderObject()) {
			o->Draw(is);
		}
	}

	void GameObject::DebugGUI() {
	#ifdef _DEBUGMODE
		// 編集UIはObject3d側(Object3dEditor)に集約されている
		if (model_) {
			model_->DebugGUI();
		}
	#endif // _DEBUG
	}

	#ifdef _DEBUGMODE
	void GameObject::Debug() {
	}
	#endif // _DEBUG

	float GameObject::ComparNum(float a, float b) {
		return (a < b) ? a : b;
	}

	void GameObject::CreateModel(const std::string& name) {
		EnsureModel()->Create(name);
	}

	void GameObject::CreateAnimeModel(const std::string& name) {
		EnsureAnimeModel()->Create(name);
	}

	void GameObject::CreateFromJson(const std::string& name) {
		nlohmann::json objJson = JsonSerializer::DeserializeJsonData(name);
		std::string modelName = objJson.value("modelName", "DefaultModel");
		EnsureModel()->Create(modelName);
		if (objJson.contains("transform")) {
			const auto& t = objJson["transform"];
			if (t.contains("translate")) {
				model_->GetTransform().translate.x = t["translate"][0];
				model_->GetTransform().translate.y = t["translate"][1];
				model_->GetTransform().translate.z = t["translate"][2];
			}
			if (t.contains("rotate")) {
				model_->GetTransform().rotate.x = t["rotate"][0];
				model_->GetTransform().rotate.y = t["rotate"][1];
				model_->GetTransform().rotate.z = t["rotate"][2];
			}
			if (t.contains("scale")) {
				model_->GetTransform().scale.x = t["scale"][0];
				model_->GetTransform().scale.y = t["scale"][1];
				model_->GetTransform().scale.z = t["scale"][2];
			}
		}
	}

	void GameObject::CreateFromJson() {
		std::string modelName = modelDataJson_->value("modelName", "DefaultModel");
		EnsureModel()->Create(modelName);
		if (modelDataJson_->contains("transform")) {
			const auto& t = (*modelDataJson_)["transform"];
			if (t.contains("translate")) {
				model_->GetTransform().translate.x = t["translate"][0];
				model_->GetTransform().translate.y = t["translate"][1];
				model_->GetTransform().translate.z = t["translate"][2];
			}
			if (t.contains("rotate")) {
				model_->GetTransform().rotate.x = t["rotate"][0];
				model_->GetTransform().rotate.y = t["rotate"][1];
				model_->GetTransform().rotate.z = t["rotate"][2];
			}
			if (t.contains("scale")) {
				model_->GetTransform().scale.x = t["scale"][0];
				model_->GetTransform().scale.y = t["scale"][1];
				model_->GetTransform().scale.z = t["scale"][2];
			}
		}
	}

	void GameObject::SetModel(const std::string& name) {
		EnsureModel()->SetModel(name);
	}

	void GameObject::SetAnimeModel(const std::string& name) {
		EnsureAnimeModel()->SetModel(name);
	}

	void GameObject::SetModelDataJson(const nlohmann::json& jsonData) {
		*modelDataJson_ = jsonData;
	}

	Graphics::Object3d* GameObject::AddRenderer() {
		auto obj = std::make_unique<Object3d>();
		Object3d* handle = obj.get();
		renderers_.push_back({ std::move(obj), true });
		return handle;
	}

	Graphics::Object3d* GameObject::AddRenderer(const std::string& name) {
		Object3d* handle = AddRenderer();
		handle->Create(name);
		return handle;
	}

	Math::Trans* GameObject::AddAnchor() {
		auto anchor = std::make_unique<Trans>();
		anchor->scale = { 1.0f,1.0f,1.0f };
		Trans* handle = anchor.get();
		anchors_.push_back(std::move(anchor));
		return handle;
	}

	AABBCollider* GameObject::AddCollider(const std::string& tag) {
		auto collider = std::make_unique<AABBCollider>();
		AABBCollider* handle = collider.get();

		handle->SetTag(tag);
		handle->SetOwner(this);
		handle->SetCollisionEnterCallback([this](const ColliderInfo& other) { OnCollisionEnter(other); });
		handle->SetCollisionStayCallback([this](const ColliderInfo& other) { OnCollisionStay(other); });
		handle->SetCollisionExitCallback([this](const ColliderInfo& other) { OnCollisionExit(other); });

		colliders_.push_back(std::move(collider));
		return handle;
	}

	void GameObject::UpdateColliders() {
		for (auto& collider : colliders_) {
			collider->InfoUpdate();
		}
	}

	void GameObject::DrawColliders() {
	#ifdef _DEBUGMODE
		for (auto& collider : colliders_) {
			collider->DrawCollider();
		}
	#endif // _DEBUG
	}

	void GameObject::SetRendererVisible(const Graphics::RenderObject* handle, bool visible) {
		for (auto& r : renderers_) {
			if (r.object.get() == handle) {
				r.visible = visible;
				return;
			}
		}
	}

}
