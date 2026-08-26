#include "GameObject.h"
#include <json.hpp>
#include "Engine/Core/Serialize/JsonSerializer.h"
#include "Engine/Core/Debug/ImGuiManager.h"

using namespace Collision;

// ★名前空間とクラス名が同名なので using namespace GameObject; は書けない(曖昧になる)。
// 定義は名前空間ブロックで囲む
namespace GameObject {

	using namespace Core;
	using namespace Graphics;
	using namespace Math;
	using namespace Editor;


	GameObject::GameObject() : modelDataJson_(std::make_unique<nlohmann::json>()) {
		// Math::Trans の既定は scale=0。1 にしておかないと子の GetWorldMat が潰れる
		transform_.scale = { 1.0f,1.0f,1.0f };
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
			auto object = std::make_unique<Object3d>();
			model_ = object.get();
			// 見た目はこのオブジェクトの位置にぶら下がる。モデル側のTransformはローカルオフセット用に空けておく
			model_->SetParent(&transform_);
			RegisterRenderer(std::move(object), true);
		}
		return model_;
	}

	Graphics::AnimationModel* GameObject::EnsureAnimeModel() {
		if (!animeModel_) {
			auto object = std::make_unique<AnimationModel>();
			animeModel_ = object.get();
			animeModel_->SetParent(&transform_);
			RegisterRenderer(std::move(object), true);
		}
		return animeModel_;
	}

	void GameObject::Update() {
		// 基底が持つコンポーネントの毎フレーム更新はここに集約する。
		// 派生は自分の処理を書いた最後にこれを呼ぶこと(位置を動かしてから当たり判定に反映させるため)
		UpdateColliders();
	}

	void GameObject::Draw(bool is) {
		// renderers_ の並びがそのまま描画順。主ビジュアルは最後尾に入っている
		for (auto& r : renderers_) {
			if (r.visible) {
				r.object->Draw(is);
			}
		}
	}

	void GameObject::DebugGUI() {
	#ifdef _DEBUGMODE
		// オブジェクト自身のTransform。モデル側のギズモが動かすのはローカルオフセットなので、
		// 本体(コライダーやエミッタを含む)を動かすにはこちらを触ること
		if (ImGui::TreeNode("Transform")) {
			ImGui::DragFloat3("translate", &transform_.translate.x, 0.01f);
			ImGui::DragFloat3("rotate", &transform_.rotate.x, 0.01f);
			ImGui::DragFloat3("scale", &transform_.scale.x, 0.01f);
			ImGui::TreePop();
		}

		// 見た目の編集UIは Object3dEditor / AnimationModelEditor に集約されている
		if (model_) {
			model_->DebugGUI();
		} else if (animeModel_) {
			animeModel_->DebugGUI();
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
				transform_.translate.x = t["translate"][0];
				transform_.translate.y = t["translate"][1];
				transform_.translate.z = t["translate"][2];
			}
			if (t.contains("rotate")) {
				transform_.rotate.x = t["rotate"][0];
				transform_.rotate.y = t["rotate"][1];
				transform_.rotate.z = t["rotate"][2];
			}
			if (t.contains("scale")) {
				transform_.scale.x = t["scale"][0];
				transform_.scale.y = t["scale"][1];
				transform_.scale.z = t["scale"][2];
			}
		}
	}

	void GameObject::CreateFromJson() {
		std::string modelName = modelDataJson_->value("modelName", "DefaultModel");
		EnsureModel()->Create(modelName);
		if (modelDataJson_->contains("transform")) {
			const auto& t = (*modelDataJson_)["transform"];
			if (t.contains("translate")) {
				transform_.translate.x = t["translate"][0];
				transform_.translate.y = t["translate"][1];
				transform_.translate.z = t["translate"][2];
			}
			if (t.contains("rotate")) {
				transform_.rotate.x = t["rotate"][0];
				transform_.rotate.y = t["rotate"][1];
				transform_.rotate.z = t["rotate"][2];
			}
			if (t.contains("scale")) {
				transform_.scale.x = t["scale"][0];
				transform_.scale.y = t["scale"][1];
				transform_.scale.z = t["scale"][2];
			}
		}
	}

	void GameObject::LoadTransformFromJson(const std::string& name) {
		JsonSerializer::DeserializeTransform(name, transform_);
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
		auto object = std::make_unique<Object3d>();
		Object3d* handle = object.get();
		RegisterRenderer(std::move(object), false);
		return handle;
	}

	void GameObject::RegisterRenderer(std::unique_ptr<Graphics::RenderObject> object, bool isPrimary) {
		const bool hasPrimary = (model_ != nullptr || animeModel_ != nullptr);
		if (isPrimary || !hasPrimary) {
			renderers_.push_back({ std::move(object), true });
			return;
		}
		// 主ビジュアルは最後尾のままにする(子は本体より前に描く)
		renderers_.insert(renderers_.end() - 1, RendererEntry{ std::move(object), true });
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
