#include "ContentBrowser.h"
#ifdef _DEBUGMODE
#include <algorithm>
#include <cctype>
#include <filesystem>
#include "Engine/Core/App/MyWindow.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/Editor/ObjectEditor/GameObjectEditor.h"
#include "Engine/GameObject/PlacedObject.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Texture/TextureManager.h"
#include "Engine/Math/Matrix/MatrixCalculation.h"
#include "Engine/Scene/Level.h"

using namespace Core;
using namespace Math;

namespace {

	constexpr const char* kModelPayload = "FUJI_ASSET_MODEL";
	constexpr const char* kTexturePayload = "FUJI_ASSET_TEXTURE";
	constexpr const char* kModelDirectory = "resource/Models";
	constexpr const char* kTextureDirectory = "resource/Textures";
	// 読み込んでいない縮小画像を1フレームに読む数。開いた途端に止まらないように
	constexpr int kThumbnailLoadsPerFrame = 2;
	// 地面に当たらないときに置く、カメラからの距離
	constexpr float kFallbackDistance = 10.0f;

	std::string Lower(std::string text) {
		std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return text;
	}

	bool HasExtension(const std::filesystem::path& path, std::initializer_list<const char*> extensions) {
		const std::string extension = Lower(path.extension().string());
		return std::any_of(extensions.begin(), extensions.end(), [&extension](const char* e) { return extension == e; });
	}

	/// <summary>マウスの位置の下にある地面 (高さ0) の点。当たらなければカメラの前</summary>
	Vector3 GroundPointUnderMouse(float mouseX, float mouseY) {
		Graphics::Camera* camera = Graphics::CameraManager::GetInstance()->GetCamera();
		if (camera == nullptr) {
			return {};
		}
		// ゲーム画面のパネルの中の位置。パネルが無ければウィンドウ全体
		const MyWin::GameView& view = MyWin::GetGameView();
		float x = 0.0f, y = 0.0f, width = 0.0f, height = 0.0f;
		if (view.isOnPanel && view.width > 0.0f && view.height > 0.0f) {
			x = view.x; y = view.y; width = view.width; height = view.height;
		} else {
			const ImVec2 size = ImGui::GetIO().DisplaySize;
			width = size.x; height = size.y;
		}
		const float ndcX = (mouseX - x) / width * 2.0f - 1.0f;
		const float ndcY = 1.0f - (mouseY - y) / height * 2.0f;
		const Matrix4x4 inverseViewProj = Inverse(camera->GetViewProjectionMatrix());
		const Vector3 nearPoint = Transform({ ndcX, ndcY, 0.0f }, inverseViewProj);
		const Vector3 farPoint = Transform({ ndcX, ndcY, 1.0f }, inverseViewProj);
		const Vector3 direction = (farPoint - nearPoint).Normalize();
		if (direction.y < -1e-4f) {
			const float t = -nearPoint.y / direction.y;
			if (t > 0.0f && t < camera->GetFarClip()) {
				return nearPoint + direction * t;
			}
		}
		return nearPoint + direction * kFallbackDistance;
	}
}

namespace Editor {

	void ContentBrowser::Scan() {
		auto scan = [](const char* directory, std::initializer_list<const char*> extensions) {
			std::vector<Asset> assets;
			std::error_code error;
			const std::filesystem::path root = directory;
			if (!std::filesystem::exists(root, error)) {
				return assets;
			}
			for (auto it = std::filesystem::recursive_directory_iterator(root, error);
				it != std::filesystem::recursive_directory_iterator(); it.increment(error)) {
				if (error || !it->is_regular_file() || !HasExtension(it->path(), extensions)) {
					continue;
				}
				// 置物の model / texture と同じく、区切りは / の相対パス
				const std::filesystem::path relative = std::filesystem::relative(it->path(), root, error);
				Asset asset;
				asset.name = relative.generic_string();
				asset.folder = relative.parent_path().generic_string();
				assets.push_back(std::move(asset));
			}
			// いちばん上を先に、あとはフォルダの名前順
			std::sort(assets.begin(), assets.end(), [](const Asset& a, const Asset& b) {
				if (a.folder != b.folder) {
					return a.folder.empty() || (!b.folder.empty() && a.folder < b.folder);
				}
				return a.name < b.name;
			});
			return assets;
		};
		models_ = scan(kModelDirectory, { ".obj", ".gltf" });
		textures_ = scan(kTextureDirectory, { ".png", ".jpg" });
		isScanned_ = true;
	}

	void ContentBrowser::WindowGUI() {
		if (!isScanned_) {
			Scan();
		}
		thumbnailLoadsLeft_ = kThumbnailLoadsPerFrame;

		if (ImGui::Button("Refresh")) {
			Scan();
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
		ImGui::InputTextWithHint("##filter", "Filter", filter_, sizeof(filter_));
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::SliderFloat("##size", &tileSize_, 40.0f, 160.0f, "%.0f px");

		if (ImGui::BeginTabBar("##content")) {
			if (ImGui::BeginTabItem("Models")) {
				if (ImGui::BeginChild("##models")) {
					AssetList(models_, true);
				}
				ImGui::EndChild();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Textures")) {
				if (ImGui::BeginChild("##textures")) {
					AssetList(textures_, false);
				}
				ImGui::EndChild();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}

	void ContentBrowser::AssetList(const std::vector<Asset>& assets, bool isModel) {
		const std::string filter = Lower(filter_);
		const float cell = tileSize_ + ImGui::GetStyle().ItemSpacing.x;
		const int columns = (std::max)(1, static_cast<int>(ImGui::GetContentRegionAvail().x / cell));
		const std::string* currentFolder = nullptr;
		bool isOpen = true;
		int column = 0;
		for (const Asset& asset : assets) {
			if (!filter.empty() && Lower(asset.name).find(filter) == std::string::npos) {
				continue;
			}
			if (currentFolder == nullptr || asset.folder != *currentFolder) {
				currentFolder = &asset.folder;
				column = 0;
				// サブフォルダは閉じた状態から。開くまで縮小画像を読まない
				const char* label = asset.folder.empty() ? "/" : asset.folder.c_str();
				isOpen = ImGui::CollapsingHeader(label, asset.folder.empty() ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None);
			}
			if (!isOpen) {
				continue;
			}
			if (column > 0) {
				ImGui::SameLine();
			}
			AssetTile(asset, isModel);
			column = (column + 1) % columns;
		}
	}

	void ContentBrowser::AssetTile(const Asset& asset, bool isModel) {
		ImGui::PushID(asset.name.c_str());
		ImGui::BeginGroup();
		const ImVec2 size(tileSize_, tileSize_);
		if (isModel) {
			ImGui::Button("3D", size);
		} else {
			Graphics::TextureManager* textureManager = Graphics::TextureManager::GetInstance();
			Graphics::Texture* texture = textureManager->FindLoadedTexture(asset.name);
			if (texture == nullptr && thumbnailLoadsLeft_ > 0 && ImGui::IsRectVisible(size)) {
				texture = textureManager->GetTexture(asset.name);
				--thumbnailLoadsLeft_;
			}
			if (texture) {
				ImGui::ImageButton("##tile", static_cast<ImTextureID>(texture->gpuHandle.ptr), size);
			} else {
				ImGui::Button("...", size);
			}
		}
		// ゲーム画面へドラッグして使う。モデルはダブルクリックでも画面の真ん中の地面に置ける
		if (isModel && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
			doubleClickedModel_ = asset.name;
		}
		if (ImGui::BeginDragDropSource()) {
			ImGui::SetDragDropPayload(isModel ? kModelPayload : kTexturePayload, asset.name.c_str(), asset.name.size() + 1);
			ImGui::TextUnformatted(asset.name.c_str());
			ImGui::EndDragDropSource();
		} else if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("%s", asset.name.c_str());
		}

		// タイルの幅に収まるまで名前を縮める
		std::string label = std::filesystem::path(asset.name).filename().string();
		if (ImGui::CalcTextSize(label.c_str()).x > tileSize_) {
			while (label.size() > 1 && ImGui::CalcTextSize((label + "..").c_str()).x > tileSize_) {
				label.pop_back();
			}
			label += "..";
		}
		ImGui::TextUnformatted(label.c_str());
		ImGui::EndGroup();
		ImGui::PopID();
	}

	void ContentBrowser::AcceptGameViewDrop() {
		if (!ImGui::BeginDragDropTarget()) {
			return;
		}
		for (const bool isModel : { true, false }) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(isModel ? kModelPayload : kTexturePayload)) {
				Drop drop;
				drop.name = static_cast<const char*>(payload->Data);
				drop.isModel = isModel;
				drop.mouseX = ImGui::GetIO().MousePos.x;
				drop.mouseY = ImGui::GetIO().MousePos.y;
				pendingDrop_ = drop;
			}
		}
		ImGui::EndDragDropTarget();
	}

	void ContentBrowser::ApplyDrop(Scene::Level* level, GameObjectEditor& editor) {
		if (level == nullptr) {
			pendingDrop_.reset();
			doubleClickedModel_.clear();
			return;
		}
		if (!doubleClickedModel_.empty()) {
			// 画面の真ん中の下の地面
			const MyWin::GameView& view = MyWin::GetGameView();
			const ImVec2 display = ImGui::GetIO().DisplaySize;
			const float centerX = view.isOnPanel ? view.x + view.width * 0.5f : display.x * 0.5f;
			const float centerY = view.isOnPanel ? view.y + view.height * 0.5f : display.y * 0.5f;
			GameObject::PlacedObject* placed = level->Add(doubleClickedModel_);
			placed->GetTrans().translate = GroundPointUnderMouse(centerX, centerY);
			editor.Select(placed);
			doubleClickedModel_.clear();
		}
		if (!pendingDrop_) {
			return;
		}
		const Drop drop = *pendingDrop_;
		pendingDrop_.reset();
		if (drop.isModel) {
			GameObject::PlacedObject* placed = level->Add(drop.name);
			placed->GetTrans().translate = GroundPointUnderMouse(drop.mouseX, drop.mouseY);
			editor.Select(placed);
			return;
		}
		// テクスチャは選んでいる置物に貼る
		auto* placed = dynamic_cast<GameObject::PlacedObject*>(editor.GetSelectedObject());
		if (placed && level->Contains(placed)) {
			placed->SetLookTexture(drop.name);
		}
	}

}
#endif // _DEBUGMODE
