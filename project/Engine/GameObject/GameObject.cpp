#include "GameObject.h"
#include <typeinfo>
#include <filesystem>
#include <unordered_map>
#include <json.hpp>
#include "Engine/Core/Serialize/JsonSerializer.h"
#include "Engine/Core/Debug/ImGuiManager.h"

using namespace Collision;

// 名前空間とクラス名が同名なので using namespace GameObject; は書けない
namespace GameObject {

	using namespace Core;
	using namespace Graphics;
	using namespace Math;
	using namespace Editor;


	namespace {
		// 関数内 static にして、どの翻訳単位の静的オブジェクトより先に使われても構築済みにする
		std::vector<GameObject*>& Registry() {
			static std::vector<GameObject*> registry;
			return registry;
		}

		const std::filesystem::path kParamDirectory = "resource/Json/Param";

		// 調整値のファイルはクラス単位。弾のように同じクラスが何度も生成されるので、読むのは最初の1回だけにする
		nlohmann::json& ParamFile(const std::string& typeName) {
			static std::unordered_map<std::string, nlohmann::json> cache;
			auto it = cache.find(typeName);
			if (it == cache.end()) {
				nlohmann::json data = JsonSerializer::DeserializeJsonData((kParamDirectory / (typeName + ".json")).string());
				it = cache.emplace(typeName, data.is_object() ? std::move(data) : nlohmann::json::object()).first;
			}
			return it->second;
		}

		// 型が合わない値は読まない。手で書き換えたファイルで既定値を壊さないため
		void ReadParam(const nlohmann::json& v, float& out) { if (v.is_number()) { out = v.get<float>(); } }
		void ReadParam(const nlohmann::json& v, int& out) { if (v.is_number_integer()) { out = v.get<int>(); } }
		void ReadParam(const nlohmann::json& v, bool& out) { if (v.is_boolean()) { out = v.get<bool>(); } }
		void ReadParam(const nlohmann::json& v, Vector3& out) {
			if (v.is_array() && v.size() == 3) { out = { v[0].get<float>(), v[1].get<float>(), v[2].get<float>() }; }
		}
		void ReadParam(const nlohmann::json& v, Vector4& out) {
			if (v.is_array() && v.size() == 4) { out = { v[0].get<float>(), v[1].get<float>(), v[2].get<float>(), v[3].get<float>() }; }
		}

		nlohmann::json WriteParam(float value) { return value; }
		nlohmann::json WriteParam(int value) { return value; }
		nlohmann::json WriteParam(bool value) { return value; }
		nlohmann::json WriteParam(const Vector3& value) { return { value.x, value.y, value.z }; }
		nlohmann::json WriteParam(const Vector4& value) { return { value.x, value.y, value.z, value.w }; }
	}

	GameObject::GameObject() {
		// Math::Trans の既定は scale=0。1 にしておかないと子の GetWorldMat が潰れる
		transform_.scale = { 1.0f,1.0f,1.0f };
		Registry().push_back(this);
	}

	GameObject::~GameObject() {
		std::erase(Registry(), this);
	}

	const std::vector<GameObject*>& GameObject::GetAll() {
		return Registry();
	}

	std::string GameObject::GetName() const {
		if (!name_.empty()) {
			return name_;
		}
		return GetTypeName();
	}

	std::string GameObject::GetTypeName() const {
		// MSVC は "class Foo::Bar" の形で返すので、キーワードと名前空間を落とす
		std::string name = typeid(*this).name();
		if (const size_t space = name.rfind(' '); space != std::string::npos) {
			name.erase(0, space + 1);
		}
		if (const size_t scope = name.rfind("::"); scope != std::string::npos) {
			name.erase(0, scope + 2);
		}
		return name;
	}

	bool GameObject::OwnsTrans(const Math::Trans* t) const {
		if (t == &transform_) {
			return true;
		}
		for (const auto& r : renderers_) {
			if (t == &r.object->GetTransform()) {
				return true;
			}
		}
		for (const auto& anchor : anchors_) {
			if (t == anchor.get()) {
				return true;
			}
		}
		return false;
	}

	bool GameObject::OwnsObjID(int objID) const {
		for (const auto& r : renderers_) {
			if (r.object->GetObjID() == objID) {
				return true;
			}
		}
		return false;
	}

	void GameObject::Initialize() {
		// CreateModel系が呼ばれたときに必要な方だけ生成する
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

	void GameObject::InspectorGUI() {
	#ifdef _DEBUGMODE
		// 見た目の編集UIは Object3dEditor / AnimationModelEditor に集約されている。
		// そちらのTransformはこのオブジェクトからのローカルオフセット
		if (!renderers_.empty() && ImGui::CollapsingHeader("Renderers", ImGuiTreeNodeFlags_DefaultOpen)) {
			for (size_t i = 0; i < renderers_.size(); ++i) {
				RendererEntry& r = renderers_[i];
				ImGui::PushID(static_cast<int>(i));
				ImGui::Checkbox("##visible", &r.visible);
				ImGui::SameLine();
				const bool isPrimary = (r.object.get() == model_ || r.object.get() == animeModel_);
				const std::string label = (isPrimary ? "Primary" : "Renderer " + std::to_string(i));
				if (ImGui::TreeNode(label.c_str())) {
					if (auto* object = dynamic_cast<Object3d*>(r.object.get())) {
						object->DebugGUI();
					} else if (auto* anime = dynamic_cast<AnimationModel*>(r.object.get())) {
						anime->DebugGUI();
					}
					ImGui::TreePop();
				}
				ImGui::PopID();
			}
		}

		if (!colliders_.empty() && ImGui::CollapsingHeader("Colliders", ImGuiTreeNodeFlags_DefaultOpen)) {
			for (size_t i = 0; i < colliders_.size(); ++i) {
				ImGui::PushID(static_cast<int>(i));
				colliders_[i]->DebugGUI();
				ImGui::PopID();
			}
		}

		if (ImGui::CollapsingHeader("Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
			for (Param& param : params_) {
				const char* label = param.name.c_str();
				if (auto* f = std::get_if<float*>(&param.value)) {
					ImGui::DragFloat(label, *f, param.speed);
				} else if (auto* i = std::get_if<int*>(&param.value)) {
					ImGui::DragInt(label, *i);
				} else if (auto* b = std::get_if<bool*>(&param.value)) {
					ImGui::Checkbox(label, *b);
				} else if (auto* v3 = std::get_if<Vector3*>(&param.value)) {
					ImGui::DragFloat3(label, &(*v3)->x, param.speed);
				} else if (auto* v4 = std::get_if<Vector4*>(&param.value)) {
					ImGui::ColorEdit4(label, &(*v4)->x);
				}
			}
			if (!params_.empty() && ImGui::Button("Save##params")) {
				SaveParams();
			}
			ParameterGUI();
		}
	#endif // _DEBUG
	}

	float GameObject::ComparNum(float a, float b) {
		return (a < b) ? a : b;
	}

	void GameObject::CreateModel(const std::string& name) {
		EnsureModel()->Create(name);
	}

	void GameObject::CreateAnimeModel(const std::string& name) {
		EnsureAnimeModel()->Create(name);
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

	void GameObject::AddParam(const std::string& name, float& value, float speed) {
		RegisterParam({ name, &value, speed });
	}

	void GameObject::AddParam(const std::string& name, int& value) {
		RegisterParam({ name, &value });
	}

	void GameObject::AddParam(const std::string& name, bool& value) {
		RegisterParam({ name, &value });
	}

	void GameObject::AddParam(const std::string& name, Math::Vector3& value, float speed) {
		RegisterParam({ name, &value, speed });
	}

	void GameObject::AddParam(const std::string& name, Math::Vector4& value) {
		RegisterParam({ name, &value });
	}

	void GameObject::RegisterParam(Param param) {
		const nlohmann::json& saved = ParamFile(GetTypeName());
		if (saved.contains(param.name)) {
			const nlohmann::json& v = saved[param.name];
			std::visit([&v](auto* p) { ReadParam(v, *p); }, param.value);
		}
		params_.push_back(std::move(param));
	}

	void GameObject::SaveParams() const {
		// ファイルにしか無い項目(派生側で登録をやめた等)は消さずに残す
		nlohmann::json& data = ParamFile(GetTypeName());
		for (const Param& param : params_) {
			data[param.name] = std::visit([](const auto* p) { return WriteParam(*p); }, param.value);
		}
		std::filesystem::create_directories(kParamDirectory);
		JsonSerializer::SerializeJsonData(data, (kParamDirectory / (GetTypeName() + ".json")).string());
	}

}
