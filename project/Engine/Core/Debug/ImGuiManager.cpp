#include "Engine/Core/Debug/ImGuiManager.h"

#include "Engine/DXC/DXCom.h"
#include "Engine/Core/App/MyWindow.h"
#include "Engine/DXC/Resource/SRVManager.h"
#include "Engine/Core/Input/Input.h"
#include "Engine/Core/Serialize/JsonSerializer.h"
#include "Engine/Graphics/Texture/TextureManager.h"
#ifdef _DEBUGMODE
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#include "ImGuizmo.h"
#endif // _DEBUG

using namespace Core;
using namespace Graphics;
using namespace Math;
using namespace DXC;


ImGuiManager* ImGuiManager::GetInstance() {
	static ImGuiManager instance;
	return &instance;
}

void ImGuiManager::Initialize([[maybe_unused]] MyWin* myWin, [[maybe_unused]] DXCom* dxComon) {
	dxCommon_ = dxComon;
#ifdef _DEBUGMODE

	SRVManager* srvManager = SRVManager::GetInstance();
	ID3D12DescriptorHeap* srv = srvManager->GetSRVHeap();
	srvIndex = srvManager->Allocate();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsLight();

	ImGui::SetColorEditOptions(ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueBar);

	/// font
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF("externals/imgui/FiraMono-Medium.ttf", 16.0f);

	ImFontConfig font_config;
	font_config.SizePixels = 18.0f;
	font_japanese_ = io.Fonts->AddFontFromFileTTF("externals/imgui/MPLUS1p-Medium.ttf", 18.0f, &font_config, io.Fonts->GetGlyphRangesJapanese());


	ImGui_ImplWin32_Init(myWin->GetHwnd());
	ImGui_ImplDX12_Init(
		dxCommon_->GetDevice(), static_cast<int>(dxCommon_->GetBackBufferCount()),
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, srv,
		srvManager->GetCPUDescriptorHandle(srvIndex),
		srvManager->GetGPUDescriptorHandle(srvIndex));

	ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
	platform_io;
	ImGuiIO& io2 = ImGui::GetIO();
	io2.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // Dockingを有効化

	SetupModernStyle();

	winSizeX_ = myWin->kWindowWidth;
	winSizeY_ = myWin->kWindowHeight;
#endif // _DEBUG
}

void ImGuiManager::Fin() {
	dxCommon_ = nullptr;
#ifdef _DEBUGMODE
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#endif // _DEBUG
}

void ImGuiManager::Begin() {
#ifdef _DEBUGMODE
	HWND hwnd = MyWin::GetInstance()->GetHwnd();
	RECT clientRect;
	GetClientRect(hwnd, &clientRect);
	float width = static_cast<float>(clientRect.right - clientRect.left);
	float height = static_cast<float>(clientRect.bottom - clientRect.top);

	POINT point;
	GetCursorPos(&point);
	ScreenToClient(hwnd, &point);
	float scaleX = (width > 0) ? (static_cast<float>(MyWin::kWindowWidth) / width) : 1.0f;
	float scaleY = (height > 0) ? (static_cast<float>(MyWin::kWindowHeight) / height) : 1.0f;
	Vector2 corrected = {
		point.x * scaleX,
		point.y * scaleY
	};

	ImGui::GetIO().AddMousePosEvent(corrected.x, corrected.y);

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGuizmo::BeginFrame();
	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetRect(0, 0, winSizeX_, winSizeY_);

	ImGuiIO& io = ImGui::GetIO();
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
	io.DisplaySize = ImVec2(static_cast<float>(MyWin::kWindowWidth), static_cast<float>(MyWin::kWindowHeight));

#endif // _DEBUG
}

void ImGuiManager::End() {
#ifdef _DEBUGMODE
	ImGui::Render();
#endif // _DEBUG
}

void ImGuiManager::Draw() {
	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

#ifdef _DEBUGMODE
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
#endif // _DEBUG
}

void ImGuiManager::SetFontJapanese() {
#ifdef _DEBUGMODE
	ImGui::PushFont(font_japanese_);
#endif // _DEBUG
}

void ImGuiManager::UnSetFont() {
#ifdef _DEBUGMODE
	ImGui::PopFont();
#endif // _DEBUG
}

#ifdef _DEBUGMODE
void ImGuiManager::ImGuiDragDropButton(const char* buttonLabel, const char* payloadType, const std::function<void(const ImGuiPayload* payload)>& onDrop, const std::function<void(const ImGuiPayload* payload)>& onPreview) {
#ifdef _DEBUGMODE
	if (ImGui::Button(buttonLabel)) {
		// ボタンが押されたときの処理が必要なら追加
	}

	if (ImGui::BeginDragDropTarget()) {
		ImGuiDragDropFlags flags = ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoPreviewTooltip;
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadType, flags)) {
			IM_UNUSED(payload);

			if (payload->IsDelivery()) {
				// ドロップされた瞬間にだけ処理を呼ぶ
				if (onDrop) {
					onDrop(payload);
				}
			} else {
				// プレビュー中の処理（必要であればここも引数化できる）
				if (onPreview) {
					onPreview(payload);
				}
			}
		}
		ImGui::EndDragDropTarget();
	}
#endif // _DEBUG
}

void ImGuiManager::ImGuiDragButton(const char* label, const void* payloadData, size_t payloadSize, const char* payloadType) {
#ifdef _DEBUGMODE
	if (ImGui::Button(label)) {
		// ボタンが押された場合の処理（必要であれば追加）
	}

	if (ImGui::BeginDragDropSource()) {
		ImGui::SetDragDropPayload(payloadType, payloadData, payloadSize);
		ImGui::Text("Dragging Now");
		ImGui::EndDragDropSource();
	}
#endif // _DEBUG
}
void ImGuiManager::ImGuiPopUpButton(const char* buttonLabel, const char* popupName, const std::function<void()>& contentFunc) {
#ifdef _DEBUGMODE
	if (ImGui::Button(buttonLabel)) {
		ImGui::OpenPopup(popupName);
	}

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0, 0, 0, 0.25f));

	if (ImGui::BeginPopupModal(popupName, NULL)) {
		contentFunc();

		ImGui::Separator();
		if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::EndPopup();
	}
	ImGui::PopStyleColor();
#endif // _DEBUG
}
#endif // _DEBUG


void Core::ImGuiManager::SetupModernStyle() {
#ifdef _DEBUGMODE
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	// --- 1. 形状設定 ---
	style.WindowRounding = 8.0f;  // ウィンドウの角を丸く
	style.ChildRounding = 6.0f;
	style.FrameRounding = 6.0f;  // 入力欄も丸く
	style.PopupRounding = 6.0f;
	style.ScrollbarRounding = 9.0f;
	style.GrabRounding = 6.0f;
	style.TabRounding = 6.0f;
	style.TabBarOverlineSize = 0.0f;

	style.WindowBorderSize = 0.0f;  // 枠線はなくしてフラットに
	style.FrameBorderSize = 1.0f;  // 入力欄には薄い枠線をつける
	style.PopupBorderSize = 1.0f;

	// --- 2. カラーパレット定義 ---
	const ImVec4 kTextDark = ImVec4(0.05f, 0.05f, 0.05f, 1.00f); // 真っ黒ではない濃いグレー
	const ImVec4 kTextLight = ImVec4(0.20f, 0.20f, 0.20f, 1.00f); // 無効テキストなど

	const ImVec4 kBgWindow = ImVec4(0.80f, 0.80f, 0.87f, 1.00f); // ほんの少し青みがかった白
	const ImVec4 kBgChild = ImVec4(0.80f, 0.80f, 0.80f, 1.00f); // リスト背景などは少し暗く
	const ImVec4 kBgInput = ImVec4(1.00f, 1.00f, 1.00f, 1.00f); // 入力欄は真っ白

	// アクセントカラー
	const ImVec4 kAccent = ImVec4(0.40f, 0.38f, 0.95f, 1.00f);
	const ImVec4 kAccentHover = ImVec4(0.50f, 0.48f, 0.98f, 1.00f);
	const ImVec4 kAccentActive = ImVec4(0.35f, 0.33f, 0.85f, 1.00f);

	// テキスト
	colors[ImGuiCol_Text] = kTextDark;
	colors[ImGuiCol_TextDisabled] = kTextLight;

	// ウィンドウ・背景
	colors[ImGuiCol_WindowBg] = kBgWindow;
	colors[ImGuiCol_ChildBg] = kBgChild;
	colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.98f); // ポップアップは白
	colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.06f); // 非常に薄いグレーの枠
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

	// ヘッダー（リストの選択項目など）
	colors[ImGuiCol_Header] = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.35f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.45f);
	colors[ImGuiCol_HeaderActive] = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.55f);

	// 入力エリア（Frame）
	colors[ImGuiCol_FrameBg] = kBgInput;
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);

	// タイトルバー
	colors[ImGuiCol_TitleBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);

	// ボタン
	colors[ImGuiCol_Button] = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.35f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.45f);
	colors[ImGuiCol_ButtonActive] = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.55f);

	// タブ
	colors[ImGuiCol_Tab] = ImVec4(0.92f, 0.93f, 0.94f, 1.00f); // 非アクティブ
	colors[ImGuiCol_TabHovered] = kAccentHover;
	colors[ImGuiCol_TabActive] = kAccent; // アクティブなタブは濃く
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.92f, 0.93f, 0.94f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TabDimmed] = ImVec4(0.92f, 0.93f, 0.94f, 1.00f); // 非アクティブ
	colors[ImGuiCol_TabDimmedSelected] = kAccentHover;

	// スライダー・スクロールバー・チェックマーク
	colors[ImGuiCol_CheckMark] = kAccent;
	colors[ImGuiCol_SliderGrab] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f); // つまみはグレー
	colors[ImGuiCol_SliderGrabActive] = kAccent;

	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f); // 薄いグレー
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = kAccent;

	// Docking (ドッキング時のプレビュー)
	colors[ImGuiCol_DockingPreview] = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.70f);
	colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);

	// プロットやグラフの線
	colors[ImGuiCol_PlotLines] = kAccent;
	colors[ImGuiCol_PlotLinesHovered] = kAccentHover;
	colors[ImGuiCol_PlotHistogram] = kAccent;
	colors[ImGuiCol_PlotHistogramHovered] = kAccentHover;
#endif // _DEBUG
}



#ifdef _DEBUGMODE
// ParticleGroup
void ParticleGroupSelector::Show(const std::function<void(const std::string&, bool)>& on_move) {

	const float listBoxHeight = 200.0f;

	if (ImGui::BeginTable("ParticleGroupTable", 3, ImGuiTableFlags_None)) {
		ImGui::TableSetupColumn("Emit: False", ImGuiTableColumnFlags_WidthStretch);    // Left side
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);      // Buttons
		ImGui::TableSetupColumn("Emit: True", ImGuiTableColumnFlags_WidthStretch);    // Right side
		ImGui::TableNextRow();

		ImGui::TableHeadersRow();
		ImGui::TableNextRow();

		// --- 左列（false） ---
		ImGui::TableSetColumnIndex(0);
		if (ImGui::BeginListBox("##falseList", ImVec2(-FLT_MIN, listBoxHeight))) {
			for (const auto& name : items[0]) {
				bool is_selected = (selected[0] == name);
				if (ImGui::Selectable(name.c_str(), is_selected)) {
					selected[0] = name;
				}
			}
			ImGui::EndListBox();
		}

		// --- 中央列（ボタン） ---
		ImGui::TableSetColumnIndex(1);
		{

			if (ImGui::Button(">")) {
				if (!selected[0].empty()) {
					items[1].push_back(selected[0]);
					items[0].erase(std::remove(items[0].begin(), items[0].end(), selected[0]), items[0].end());
					on_move(selected[0], true);
					selected[0].clear();
				}
			}

			if (ImGui::Button("<")) {
				if (!selected[1].empty()) {
					items[0].push_back(selected[1]);
					items[1].erase(std::remove(items[1].begin(), items[1].end(), selected[1]), items[1].end());
					on_move(selected[1], false);
					selected[1].clear();
				}
			}
		}

		// --- 右列（true） ---
		ImGui::TableSetColumnIndex(2);
		if (ImGui::BeginListBox("##trueList", ImVec2(-FLT_MIN, listBoxHeight))) {
			for (const auto& name : items[1]) {
				bool is_selected = (selected[1] == name);
				if (ImGui::Selectable(name.c_str(), is_selected)) {
					selected[1] = name;
				}
			}
			ImGui::EndListBox();
		}

		ImGui::EndTable();
	}
}
#endif // _DEBUG
