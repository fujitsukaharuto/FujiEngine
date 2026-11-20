#include "GameObjectEditor.h"
#ifdef _DEBUGMODE
#include <algorithm>
#include <cctype>
#include <cmath>
#include <numbers>
#include <string_view>
#include "Engine/GameObject/PlacedObject.h"
#include "Engine/GameObject/SpawnPoint.h"
#include "Engine/GameObject/PlacedLight.h"
#include "Engine/GameObject/GIVolume.h"
#include "Engine/GameObject/ReflectionProbe.h"
#include "Engine/GameObject/LocalFogVolume.h"
#include "Engine/GameObject/PostProcessVolume.h"
#include "Engine/GameObject/PlacedSpline.h"
#include "Engine/Scene/Level.h"
#include "Engine/Graphics/Sprite/PlacedSprite.h"
#include "Engine/Graphics/Texture/TextureManager.h"
#include "Engine/Core/App/MyWindow.h"
#include "Engine/Graphics/Model/ModelManager.h"
#include "Engine/Editor/Command/CommandManager.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/Graphics/Line/Line3dDrawer.h"
#include "Engine/Graphics/Model/Model.h"

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

	bool GameObjectEditor::SelectByObjID(int objID, int meshIndex) {
		if (objID <= 0) {
			return false;
		}
		for (Object* object : Object::GetAll()) {
			if (object->OwnsObjID(objID)) {
				// メッシュの番号は主ビジュアルのモデルのもの。子の描画物を拾ったときは選ばない
				const Graphics::RenderObject* primary = object->GetRenderObject();
				selectedMesh_ = (selectMode_ == SelectMode::Mesh && primary && primary->GetObjID() == objID) ? meshIndex : -1;
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
		Object* added = nullptr;
		if (ImGui::BeginCombo("##place", "Place", ImGuiComboFlags_WidthFitPreview | ImGuiComboFlags_HeightLargest)) {
			added = PlaceMenuItems(level);
			ImGui::EndCombo();
		}
		ImGui::SameLine();

		const bool canDuplicate = selected && level.Contains(selected);
		ImGui::BeginDisabled(!canDuplicate);
		// 文字を打っている最中の Ctrl+D は拾わない
		const bool isShortcut = !ImGui::GetIO().WantTextInput && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D, false);
		if ((ImGui::Button("Duplicate") || isShortcut) && canDuplicate) {
			added = level.Duplicate(selected);
		}
		ImGui::EndDisabled();
		if (added) {
			selected_ = added;
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
		// 保存は Remove から離して右端へ。入らなければ次の行の右端
		const char* levelName = level.GetName().c_str();
		const float saveWidth = ImGui::CalcTextSize(levelName).x + style.ItemSpacing.x + ImGui::CalcTextSize("Save").x + style.FramePadding.x * 2.0f;
		ImGui::SameLine();
		if (ImGui::GetContentRegionAvail().x < saveWidth) {
			ImGui::NewLine();
		}
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (std::max)(0.0f, ImGui::GetContentRegionAvail().x - saveWidth));
		ImGui::AlignTextToFramePadding();
		ImGui::TextDisabled("%s", levelName);
		ImGui::SameLine();
		if (ImGui::Button("Save")) {
			level.Save();
		}
		ImGui::Separator();
	}

	GameObjectEditor::Object* GameObjectEditor::PlaceMenuItems(Scene::Level& level) {
		struct Item {
			const char* category;
			const char* label;
			Object* (*place)(Scene::Level&);
		};
		// UE の Place Actors に倣って種類ごとに分ける。スポーンは原点に出るのでギズモで動かして使う
		static constexpr Item kItems[] = {
			{ "Basic", "Spawn Point", [](Scene::Level& l) -> Object* { return l.AddSpawnPoint(); } },
			{ "Basic", "Spline", [](Scene::Level& l) -> Object* { return l.AddSpline(); } },
			{ "Lights", "Point Light", [](Scene::Level& l) -> Object* { return l.AddLight(false); } },
			{ "Lights", "Spot Light", [](Scene::Level& l) -> Object* { return l.AddLight(true); } },
			{ "Lights", "Tube Light", [](Scene::Level& l) -> Object* { return l.AddShapedLight(true); } },
			{ "Lights", "Rect Light", [](Scene::Level& l) -> Object* { return l.AddShapedLight(false); } },
			{ "Volumes", "Fog Volume", [](Scene::Level& l) -> Object* { return l.AddLocalFogVolume(); } },
			{ "Volumes", "Post Process Volume", [](Scene::Level& l) -> Object* { return l.AddPostProcessVolume(); } },
			{ "Volumes", "Reflection Probe", [](Scene::Level& l) -> Object* { return l.AddReflectionProbe(); } },
			{ "Volumes", "GI Volume", [](Scene::Level& l) -> Object* { return l.AddGIVolume(); } },
		};
		Object* placed = nullptr;
		const char* category = nullptr;
		for (const Item& item : kItems) {
			if (category == nullptr || std::string_view(category) != item.category) {
				category = item.category;
				ImGui::SeparatorText(category);
			}
			if (ImGui::Selectable(item.label)) {
				placed = item.place(level);
			}
		}
		return placed;
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

		// クリックで選ぶ単位
		int selectMode = static_cast<int>(selectMode_);
		ImGui::TextUnformatted("Select");
		ImGui::SameLine();
		ImGui::RadioButton("Object##selectmode", &selectMode, static_cast<int>(SelectMode::Object));
		ImGui::SameLine();
		ImGui::RadioButton("Mesh##selectmode", &selectMode, static_cast<int>(SelectMode::Mesh));
		if (selectMode != static_cast<int>(selectMode_)) {
			selectMode_ = static_cast<SelectMode>(selectMode);
			selectedMesh_ = -1;
		}

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
				// 一覧の右クリックでも同じ物を置ける (Unity の Hierarchy と同じ)。物で埋まっていても開けるよう、行の上でも開く
				if (ImGui::BeginPopupContextWindow("##placeContext", ImGuiPopupFlags_MouseButtonRight)) {
					if (Object* placed = PlaceMenuItems(*level)) {
						Select(placed);
					}
					ImGui::EndPopup();
				}
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

		if (selectMode_ == SelectMode::Mesh && selectedMesh_ >= 0) {
			MeshInspectorGUI(selected);
		}

		selected->InspectorGUI();
		ImGui::PopID();
	}

	void GameObjectEditor::MeshInspectorGUI(Object* selected) {
		const Graphics::RenderObject* object = selected->GetRenderObject();
		if (!object || !object->GetModel()) {
			return;
		}
		const auto& meshes = object->GetModel()->GetMeshes();
		if (static_cast<size_t>(selectedMesh_) >= meshes.size()) {
			selectedMesh_ = -1;
			return;
		}
		if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Text("%d / %d", selectedMesh_, static_cast<int>(meshes.size()));
			if (static_cast<size_t>(selectedMesh_) < object->GetMaterialCount()) {
				Graphics::RenderObject* editable = selected->GetRenderObject();
				const std::string texture = editable->GetMaterial(static_cast<size_t>(selectedMesh_)).GetPathName();
				if (!texture.empty()) {
					ImGui::TextDisabled("%s", texture.c_str());
				}
			}
			selected->MeshInspectorGUI(selectedMesh_);
		}

		// 選んだメッシュを包む箱
		Vector3 min;
		Vector3 max;
		if (!meshes[static_cast<size_t>(selectedMesh_)].GetBounds(min, max)) {
			return;
		}
		const Matrix4x4 world = object->GetWorldMat();
		Vector3 corners[8];
		for (int i = 0; i < 8; i++) {
			corners[i] = Transform({ (i & 1) ? max.x : min.x, (i & 2) ? max.y : min.y, (i & 4) ? max.z : min.z }, world);
		}
		const Vector4 color = { 1.0f,0.85f,0.2f,1.0f };
		Graphics::Line3dDrawer* line = Graphics::Line3dDrawer::GetInstance();
		for (int i = 0; i < 8; i++) {
			for (int bit = 1; bit < 8; bit <<= 1) {
				if ((i & bit) == 0) {
					line->DrawLine3d(corners[i], corners[i | bit], color);
				}
			}
		}
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
