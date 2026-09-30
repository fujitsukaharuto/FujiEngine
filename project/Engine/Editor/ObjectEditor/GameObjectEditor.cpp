#include "GameObjectEditor.h"
#ifdef _DEBUGMODE
#include <algorithm>
#include <cctype>
#include <cmath>
#include <numbers>
#include "Engine/GameObject/PlacedObject.h"
#include "Engine/GameObject/SpawnPoint.h"
#include "Engine/Scene/Level.h"
#include "Engine/Graphics/Sprite/PlacedSprite.h"
#include "Engine/Graphics/Texture/TextureManager.h"
#include "Engine/Core/App/MyWindow.h"
#include "Engine/Graphics/Model/ModelManager.h"
#include "Engine/Editor/Command/CommandManager.h"
#include "Engine/Core/Debug/ImGuiManager.h"

using namespace Core;
using namespace Math;


namespace Editor {

	namespace {
		constexpr float kRadToDeg = 180.0f / std::numbers::pi_v<float>;
		constexpr float kDegToRad = std::numbers::pi_v<float> / 180.0f;

		size_t IndexOf(const std::vector<GameObject::GameObject*>& all, const GameObject::GameObject* object) {
			return static_cast<size_t>(std::find(all.begin(), all.end(), object) - all.begin());
		}
	}

	GameObjectEditor::Object* GameObjectEditor::GetSelected() {
		// 選択中に弾やエフェクトが寿命で消えることがあるので、毎回まだ生きているか確かめる
		const auto& all = Object::GetAll();
		if (selected_ && std::find(all.begin(), all.end(), selected_) == all.end()) {
			selected_ = nullptr;
		}
		return selected_;
	}

	Graphics::PlacedSprite* GameObjectEditor::GetSelectedSprite(const Scene::Level* level) {
		if (selectedSprite_ && (!level || !level->ContainsSprite(selectedSprite_))) {
			selectedSprite_ = nullptr;
			isDraggingSprite_ = false;
		}
		return selectedSprite_;
	}

	bool GameObjectEditor::IsListed(const Object* object) const {
		return showInactive_ || object->IsActive();
	}

	GameObjectEditor::Object* GameObjectEditor::FindParent(const Object* object, const std::vector<Object*>& all) {
		for (const Trans* t = object->GetTrans().parent; t; t = t->parent) {
			for (Object* other : all) {
				if (other != object && other->OwnsTrans(t)) {
					return other;
				}
			}
		}
		return nullptr;
	}

	bool GameObjectEditor::MatchFilter(const Object* object) const {
		if (filter_[0] == '\0') {
			return true;
		}
		auto lower = [](std::string s) {
			std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return s;
		};
		return lower(object->GetName()).find(lower(filter_)) != std::string::npos;
	}

	bool GameObjectEditor::SelectByObjID(int objID) {
		if (objID <= 0) {
			return false;
		}
		for (Object* object : Object::GetAll()) {
			if (object->OwnsObjID(objID)) {
				selected_ = object;
				isEnvironmentSelected_ = false;
				selectedSprite_ = nullptr;
				revealSelected_ = true;
				return true;
			}
		}
		return false;
	}

	void GameObjectEditor::LevelToolbarGUI(Scene::Level& level) {
		const ImGuiStyle& style = ImGui::GetStyle();
		ImGui::SetNextItemWidth(-(ImGui::CalcTextSize("Add").x + style.FramePadding.x * 2.0f + style.ItemSpacing.x));
		if (ImGui::BeginCombo("##model", addModel_.c_str())) {
			for (const auto& file : Graphics::ModelManager::GetInstance()->GetModelFiles()) {
				if (ImGui::Selectable(file.first.c_str(), file.first == addModel_)) {
					addModel_ = file.first;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		if (ImGui::Button("Add")) {
			selected_ = level.Add(addModel_);
			isEnvironmentSelected_ = false;
			selectedSprite_ = nullptr;
			revealSelected_ = true;
		}

		ImGui::PushID("sprite");
		ImGui::SetNextItemWidth(-(ImGui::CalcTextSize("Add").x + style.FramePadding.x * 2.0f + style.ItemSpacing.x));
		if (ImGui::BeginCombo("##texture", addTexture_.c_str())) {
			for (const auto& file : Graphics::TextureManager::GetInstance()->GetTextureFiles()) {
				if (ImGui::Selectable(file.first.c_str(), file.first == addTexture_)) {
					addTexture_ = file.first;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		if (ImGui::Button("Add")) {
			selectedSprite_ = level.AddSprite(addTexture_);
			isEnvironmentSelected_ = false;
			selected_ = nullptr;
		}
		ImGui::PopID();

		Object* selected = GetSelected();
		Graphics::PlacedSprite* selectedSprite = GetSelectedSprite(&level);
		if (ImGui::Button("Add Spawn")) {
			GameObject::SpawnPoint* point = level.AddSpawnPoint();
			// 原点に出るので、ギズモで動かして使う
			selected_ = point;
			isEnvironmentSelected_ = false;
			selectedSprite_ = nullptr;
			revealSelected_ = true;
		}
		ImGui::SameLine();
		ImGui::BeginDisabled(!selectedSprite && (!selected || !level.Contains(selected)));
		if (ImGui::Button("Remove")) {
			if (selectedSprite) {
				level.RemoveSprite(selectedSprite);
				selectedSprite_ = nullptr;
			} else {
				level.Remove(selected);
				selected_ = nullptr;
				// Undo の履歴が消した置物の Transform を握っているので捨てる
				CommandManager::GetInstance()->StackReset();
			}
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Save")) {
			level.Save();
		}
		ImGui::SameLine();
		ImGui::TextDisabled("%s", level.GetName().c_str());
		ImGui::Separator();
	}

	void GameObjectEditor::HierarchyGUI(Scene::Level* level) {
		if (level) {
			LevelToolbarGUI(*level);
			if (ImGui::Selectable("Environment", isEnvironmentSelected_)) {
				isEnvironmentSelected_ = true;
				selected_ = nullptr;
				selectedSprite_ = nullptr;
			}
			ImGui::Separator();
		} else {
			isEnvironmentSelected_ = false;
		}

		Object* selected = GetSelected();
		const std::vector<Object*>& all = Object::GetAll();

		const char* inactiveLabel = "非アクティブ";
		const ImGuiStyle& style = ImGui::GetStyle();
		ImGui::SetNextItemWidth(-(ImGui::GetFrameHeight() + style.ItemInnerSpacing.x + ImGui::CalcTextSize(inactiveLabel).x + style.ItemSpacing.x));
		ImGui::InputTextWithHint("##filter", "Filter", filter_, sizeof(filter_));
		ImGui::SameLine();
		ImGui::Checkbox(inactiveLabel, &showInactive_);
		ImGui::Separator();

		// 親は毎フレーム引き直す。ペアレントはゲーム中に付け替えられることがある
		std::vector<Object*> parents(all.size());
		for (size_t i = 0; i < all.size(); ++i) {
			parents[i] = FindParent(all[i], all);
		}

		// ピックで選ばれたときは、祖先を開いて見える所まで出す
		revealPath_.clear();
		if (revealSelected_ && selected) {
			for (Object* p = parents[IndexOf(all, selected)]; p; p = parents[IndexOf(all, p)]) {
				revealPath_.push_back(p);
			}
		}

		if (ImGui::BeginChild("##hierarchy")) {
			if (filter_[0] != '\0') {
				// 絞り込み中は木を畳まずに平たく並べる。深い所の子を探すのが目的なので
				for (Object* object : all) {
					if (IsListed(object) && MatchFilter(object)) {
						ImGui::PushID(object);
						ImGui::BeginDisabled(!object->IsActive());
						if (ImGui::Selectable(object->GetName().c_str(), object == selected)) {
							selected_ = object;
							isEnvironmentSelected_ = false;
							selectedSprite_ = nullptr;
						}
						ImGui::EndDisabled();
						ImGui::PopID();
					}
				}
			} else {
				for (size_t i = 0; i < all.size(); ++i) {
					if (!parents[i] && IsListed(all[i])) {
						DrawNode(all[i], all, parents);
					}
				}
			}
			if (level) {
				SpriteListGUI(*level);
			}
		}
		ImGui::EndChild();
		revealSelected_ = false;
	}

	void GameObjectEditor::DrawNode(Object* object, const std::vector<Object*>& all, const std::vector<Object*>& parents) {
		bool hasChild = false;
		for (size_t i = 0; i < all.size(); ++i) {
			if (parents[i] == object && IsListed(all[i])) {
				hasChild = true;
				break;
			}
		}

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (!hasChild) {
			flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		}
		if (object == selected_) {
			flags |= ImGuiTreeNodeFlags_Selected;
		}
		if (std::find(revealPath_.begin(), revealPath_.end(), object) != revealPath_.end()) {
			ImGui::SetNextItemOpen(true);
		}

		// 止めているものは薄く出す。押せなくすると選べないので色だけ変える
		if (!object->IsActive()) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
		}
		const bool isOpen = ImGui::TreeNodeEx(object, flags, "%s", object->GetName().c_str());
		if (!object->IsActive()) {
			ImGui::PopStyleColor();
		}
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
			selected_ = object;
			isEnvironmentSelected_ = false;
			selectedSprite_ = nullptr;
		}
		if (object == selected_ && revealSelected_) {
			ImGui::SetScrollHereY();
		}

		if (isOpen && hasChild) {
			for (size_t i = 0; i < all.size(); ++i) {
				if (parents[i] == object && IsListed(all[i])) {
					DrawNode(all[i], all, parents);
				}
			}
			ImGui::TreePop();
		}
	}

	void GameObjectEditor::InspectorGUI(Scene::Level* level) {
		if (isEnvironmentSelected_ && level) {
			ImGui::Text("Environment");
			ImGui::Separator();
			level->EnvironmentGUI();
			return;
		}

		if (Graphics::PlacedSprite* sprite = GetSelectedSprite(level)) {
			ImGui::PushID(sprite);
			ImGui::Text("%s", sprite->GetName().c_str());
			ImGui::Separator();
			sprite->InspectorGUI();
			ImGui::PopID();
			return;
		}

		Object* selected = GetSelected();
		if (!selected) {
			ImGui::TextDisabled("(未選択)");
			return;
		}

		ImGui::PushID(selected);
		bool isActive = selected->IsActive();
		if (ImGui::Checkbox("##active", &isActive)) {
			selected->SetActive(isActive);
		}
		ImGui::SameLine();
		ImGui::Text("%s", selected->GetName().c_str());
		ImGui::Separator();

		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
			Trans& t = selected->GetTrans();
			ImGui::DragFloat3("Position", &t.translate.x, 0.01f);
			Vector3 degrees = t.rotate * kRadToDeg;
			if (ImGui::DragFloat3("Rotation", &degrees.x, 0.5f)) {
				t.rotate = degrees * kDegToRad;
			}
			ImGui::DragFloat3("Scale", &t.scale.x, 0.01f);
			if (t.parent) {
				const Vector3 world = t.GetWorldPos();
				ImGui::TextDisabled("World %.2f, %.2f, %.2f", world.x, world.y, world.z);
			}
			gizmo_.DrawOperationRadio();
			gizmo_.Manipulate(t, level && level->Contains(selected));
		}

		selected->InspectorGUI();
		ImGui::PopID();
	}

	void GameObjectEditor::SpriteListGUI(Scene::Level& level) {
		const auto& sprites = level.GetSprites();
		if (sprites.empty()) {
			return;
		}
		ImGui::SeparatorText("Sprites");
		const Graphics::PlacedSprite* selected = GetSelectedSprite(&level);
		for (const auto& sprite : sprites) {
			if (filter_[0] != '\0') {
				std::string name = sprite->GetName();
				std::string filter = filter_;
				auto lower = [](std::string& s) { std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); }); };
				lower(name);
				lower(filter);
				if (name.find(filter) == std::string::npos) {
					continue;
				}
			}
			ImGui::PushID(sprite.get());
			// 非表示のものは薄く出す
			if (!sprite->IsVisible()) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
			}
			if (ImGui::Selectable(sprite->GetName().c_str(), sprite.get() == selected)) {
				selectedSprite_ = sprite.get();
				isEnvironmentSelected_ = false;
				selected_ = nullptr;
			}
			if (!sprite->IsVisible()) {
				ImGui::PopStyleColor();
			}
			ImGui::PopID();
		}
	}

	void GameObjectEditor::GameViewGUI(const Scene::Level* level) {
		Graphics::PlacedSprite* sprite = GetSelectedSprite(level);
		const MyWin::GameView& view = MyWin::GetGameView();
		if (!sprite || !view.isOnPanel) {
			isDraggingSprite_ = false;
			MyWin::SetGameViewCaptured(false);
			return;
		}

		// パネルのゲーム画面は縮小して出しているので、スプライトの座標(ゲーム画面のピクセル)へ換算する
		const float toGameX = static_cast<float>(MyWin::kWindowWidth) / view.width;
		const float toGameY = static_cast<float>(MyWin::kWindowHeight) / view.height;
		const ImGuiIO& io = ImGui::GetIO();
		const Vector2 mouse = { (io.MousePos.x - view.x) * toGameX, (io.MousePos.y - view.y) * toGameY };

		const bool isOver = view.isHovered && sprite->Contains(mouse);
		if (isOver && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			isDraggingSprite_ = true;
		}
		if (isDraggingSprite_) {
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
				const Vector2 pos = sprite->GetPos();
				sprite->SetPos({ pos.x + io.MouseDelta.x * toGameX, pos.y + io.MouseDelta.y * toGameY });
			} else {
				// 縮小表示のせいで端数が付くので、離したときにピクセルへ揃える
				const Vector2 pos = sprite->GetPos();
				sprite->SetPos({ std::round(pos.x), std::round(pos.y) });
				isDraggingSprite_ = false;
			}
		}
		MyWin::SetGameViewCaptured(isOver || isDraggingSprite_);

		// 選択中の枠。非表示のスプライトも位置が分かるように出す
		Vector2 min, max;
		sprite->GetRect(min, max);
		const ImVec2 screenMin = { view.x + min.x / toGameX, view.y + min.y / toGameY };
		const ImVec2 screenMax = { view.x + max.x / toGameX, view.y + max.y / toGameY };
		ImDrawList* drawList = ImGui::GetForegroundDrawList();
		drawList->PushClipRect(ImVec2(view.x, view.y), ImVec2(view.x + view.width, view.y + view.height), true);
		const ImU32 color = ImGui::GetColorU32(sprite->IsVisible() ? ImGuiCol_PlotHistogram : ImGuiCol_TextDisabled);
		drawList->AddRect(screenMin, screenMax, color, 0.0f, 0, (isOver || isDraggingSprite_) ? 2.0f : 1.0f);
		drawList->PopClipRect();
	}
}
#endif // _DEBUGMODE
