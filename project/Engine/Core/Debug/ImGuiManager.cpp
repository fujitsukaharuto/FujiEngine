#include "Engine/Core/Debug/ImGuiManager.h"

#include <cmath>
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

#ifdef _DEBUGMODE
namespace {

	float SrgbToLinear(float value) {
		return value <= 0.04045f ? value / 12.92f : std::pow((value + 0.055f) / 1.055f, 2.4f);
	}

	// バックバッファのRTVがsRGBで、ImGuiのシェーダは変換しない。書き込み時のエンコードで
	// 持ち上がるぶんをここで戻さないと、指定した色より明るく出る
	void LinearizeStyleColors() {
		ImVec4* colors = ImGui::GetStyle().Colors;
		for (int i = 0; i < ImGuiCol_COUNT; ++i) {
			colors[i].x = SrgbToLinear(colors[i].x);
			colors[i].y = SrgbToLinear(colors[i].y);
			colors[i].z = SrgbToLinear(colors[i].z);
		}
	}

	ImVec4 SrgbColor(float r, float g, float b) {
		return ImVec4(SrgbToLinear(r), SrgbToLinear(g), SrgbToLinear(b), 1.0f);
	}
}
#endif // _DEBUG


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

	//========================================================================*/
	//* 形状と余白
	style.WindowRounding = 6.0f;
	style.ChildRounding = 6.0f;
	style.FrameRounding = 4.0f;
	style.PopupRounding = 6.0f;
	style.ScrollbarRounding = 9.0f;
	style.GrabRounding = 4.0f;
	style.TabRounding = 6.0f;
	style.TabBarOverlineSize = 2.0f;	// 選択中のタブはこの下線で示す

	style.WindowBorderSize = 1.0f;
	style.ChildBorderSize = 1.0f;
	style.FrameBorderSize = 1.0f;
	style.PopupBorderSize = 1.0f;
	style.SeparatorTextBorderSize = 2.0f;

	style.WindowPadding = ImVec2(10.0f, 8.0f);
	style.FramePadding = ImVec2(8.0f, 4.0f);
	style.ItemSpacing = ImVec2(8.0f, 6.0f);
	style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
	style.IndentSpacing = 18.0f;
	style.ScrollbarSize = 13.0f;
	style.GrabMinSize = 10.0f;

	//========================================================================*/
	//* 配色
	// 面の明るさは 0.72 / 0.89 / 0.94 / 1.00 の4段。詰めると隣り合った面が見分けられなくなる
	const ImVec4 kBgTitle = ImVec4(0.72f, 0.72f, 0.77f, 1.00f);
	const ImVec4 kBgWindow = ImVec4(0.89f, 0.89f, 0.92f, 1.00f);
	const ImVec4 kBgChild = ImVec4(0.94f, 0.94f, 0.96f, 1.00f);
	const ImVec4 kBgInput = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	const ImVec4 kBorder = ImVec4(0.66f, 0.66f, 0.72f, 1.00f);

	const ImVec4 kText = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
	const ImVec4 kTextDisabled = ImVec4(0.40f, 0.40f, 0.46f, 1.00f);

	const ImVec4 kAccent = ImVec4(0.33f, 0.30f, 0.83f, 1.00f);
	const ImVec4 kAccentHover = ImVec4(0.43f, 0.40f, 0.92f, 1.00f);
	const ImVec4 kAccentActive = ImVec4(0.24f, 0.22f, 0.68f, 1.00f);

	// 文字は濃いままなので、面を塗るアクセントはアルファで敷く
	auto accentAlpha = [&kAccent](float alpha) {
		return ImVec4(kAccent.x, kAccent.y, kAccent.z, alpha);
		};

	colors[ImGuiCol_Text] = kText;
	colors[ImGuiCol_TextDisabled] = kTextDisabled;
	colors[ImGuiCol_TextSelectedBg] = accentAlpha(0.35f);
	colors[ImGuiCol_TextLink] = kAccent;

	colors[ImGuiCol_WindowBg] = kBgWindow;
	colors[ImGuiCol_ChildBg] = kBgChild;
	colors[ImGuiCol_PopupBg] = ImVec4(0.98f, 0.98f, 1.00f, 0.98f);
	colors[ImGuiCol_MenuBarBg] = kBgTitle;
	colors[ImGuiCol_Border] = kBorder;
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.25f);

	// アクティブなウィンドウは灰色ではなく紫寄りにして、非アクティブと区別する
	colors[ImGuiCol_TitleBg] = kBgTitle;
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.62f, 0.60f, 0.84f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.72f, 0.72f, 0.77f, 0.70f);

	colors[ImGuiCol_FrameBg] = kBgInput;
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.92f, 0.92f, 0.99f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.86f, 0.86f, 0.98f, 1.00f);

	// ヘッダ(見出し・選択項目)とボタンはアルファの濃さで区別する。同じ値にすると押せる物が分からない
	colors[ImGuiCol_Header] = accentAlpha(0.16f);
	colors[ImGuiCol_HeaderHovered] = accentAlpha(0.26f);
	colors[ImGuiCol_HeaderActive] = accentAlpha(0.36f);

	colors[ImGuiCol_Button] = accentAlpha(0.50f);
	colors[ImGuiCol_ButtonHovered] = accentAlpha(0.66f);
	colors[ImGuiCol_ButtonActive] = accentAlpha(0.80f);

	// ドッキング中のタブバーの下地は TitleBg/TitleBgActive。タブはその上に載るので下地より明るくする
	// TabActive/TabUnfocused/TabUnfocusedActive は 1.90.9 で下の3つへ改名されたエイリアス
	colors[ImGuiCol_Tab] = ImVec4(0.82f, 0.82f, 0.86f, 1.00f);
	colors[ImGuiCol_TabHovered] = accentAlpha(0.40f);
	colors[ImGuiCol_TabSelected] = kBgWindow;
	colors[ImGuiCol_TabSelectedOverline] = kAccent;
	colors[ImGuiCol_TabDimmed] = ImVec4(0.83f, 0.83f, 0.86f, 1.00f);
	colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.89f, 0.89f, 0.91f, 1.00f);
	colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.62f, 0.62f, 0.66f, 1.00f);

	colors[ImGuiCol_CheckMark] = kAccent;
	colors[ImGuiCol_SliderGrab] = ImVec4(0.55f, 0.55f, 0.62f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = kAccent;

	colors[ImGuiCol_Separator] = kBorder;
	colors[ImGuiCol_SeparatorHovered] = kAccentHover;
	colors[ImGuiCol_SeparatorActive] = kAccentActive;

	colors[ImGuiCol_ResizeGrip] = accentAlpha(0.30f);
	colors[ImGuiCol_ResizeGripHovered] = accentAlpha(0.50f);
	colors[ImGuiCol_ResizeGripActive] = accentAlpha(0.75f);

	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.82f, 0.82f, 0.86f, 0.70f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.68f, 0.68f, 0.74f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.58f, 0.58f, 0.66f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = kAccent;

	colors[ImGuiCol_TableHeaderBg] = ImVec4(0.82f, 0.82f, 0.86f, 1.00f);
	colors[ImGuiCol_TableBorderStrong] = ImVec4(0.64f, 0.64f, 0.70f, 1.00f);
	colors[ImGuiCol_TableBorderLight] = ImVec4(0.78f, 0.78f, 0.83f, 1.00f);
	colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.00f, 0.00f, 0.00f, 0.05f);

	colors[ImGuiCol_DockingPreview] = accentAlpha(0.70f);
	colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.84f, 0.84f, 0.87f, 1.00f);

	colors[ImGuiCol_NavCursor] = kAccent;
	colors[ImGuiCol_NavWindowingHighlight] = accentAlpha(0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.20f, 0.20f, 0.22f, 0.20f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(0.95f, 0.55f, 0.10f, 1.00f);

	colors[ImGuiCol_PlotLines] = kAccent;
	colors[ImGuiCol_PlotLinesHovered] = kAccentHover;
	colors[ImGuiCol_PlotHistogram] = kAccent;
	colors[ImGuiCol_PlotHistogramHovered] = kAccentHover;

	LinearizeStyleColors();
#endif // _DEBUG
}

#ifdef _DEBUGMODE
ImVec4 Core::ImGuiTextOk() { return SrgbColor(0.08f, 0.44f, 0.18f); }
ImVec4 Core::ImGuiTextWarn() { return SrgbColor(0.62f, 0.32f, 0.00f); }
ImVec4 Core::ImGuiTextError() { return SrgbColor(0.78f, 0.10f, 0.10f); }
#endif // _DEBUGMODE



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
