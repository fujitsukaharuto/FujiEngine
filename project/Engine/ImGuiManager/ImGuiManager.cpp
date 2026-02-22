#include "ImGuiManager.h"

#include "DXCom.h"
#include "MyWindow.h"
#include "SRVManager.h"
#include "Engine/Input/Input.h"
#include "Engine/Editor/JsonSerializer.h"
#include "Engine/Model/TextureManager.h"
#ifdef _DEBUGMODE
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#include "ImGuizmo.h"
#include "imgui_node_editor.h"
#include "utilities/widgets.h"

namespace ed = ax::NodeEditor;
#endif // _DEBUG

using namespace Core;
using namespace Graphics;
using namespace Math;


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
	if (nodeEditorContext_) {
		ax::NodeEditor::DestroyEditor(nodeEditorContext_);
		nodeEditorContext_ = nullptr;
	}

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
void ImGuiManager::ImGuiPopUpButton(const char* buttonLabel, const char* popupName, std::function<void()> contentFunc) {
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

#ifdef _DEBUGMODE
void ImGuiManager::InitNodeTexture() {
	backGroundHandle_ = TextureManager::GetInstance()->GetTexture("BlueprintBackground.png")->gpuHandle;
}
#endif // _DEBUG

#ifdef _DEBUGMODE
bool ImGuiManager::CanCreateLink(const Pin& a, const Pin& b) {
	// 出力 → 入力 のみに限定する例
	if (a.type == b.type || a.pinType != b.pinType)
		return false;
	return (a.type == Pin::Type::Output) ? true : false;
}

const MyNode* ImGuiManager::FindNodeByPinId(const ed::PinId& pinId, const std::vector<MyNode>& nodes) {
	for (const auto& node : nodes) {
		for (const auto& pin : node.inputs)
			if (pin.id == pinId)
				return &node;
		for (const auto& pin : node.outputs)
			if (pin.id == pinId)
				return &node;
	}
	return nullptr;
}

const Pin* ImGuiManager::FindPin(const ed::PinId& id, const std::vector<MyNode>& nodes) {
	for (const auto& node : nodes) {
		for (const auto& pin : node.inputs)
			if (pin.id == id)
				return &pin;
		for (const auto& pin : node.outputs)
			if (pin.id == id)
				return &pin;
	}
	return nullptr;
}

void ImGuiManager::HandleCreateLink(std::vector<Link>& links, const std::vector<MyNode>& nodes) {
	if (ed::BeginCreate()) {
		ed::PinId inputId, outputId;
		if (ed::QueryNewLink(&outputId, &inputId)) {
			auto* inPin = FindPin(inputId, nodes);
			auto* outPin = FindPin(outputId, nodes);

			const MyNode* inNode = FindNodeByPinId(inputId, nodes);
			const MyNode* outNode = FindNodeByPinId(outputId, nodes);

			if (inPin && outPin && inNode != outNode && CanCreateLink(*outPin, *inPin) && !inPin->isLinked && !outPin->isLinked) {
				if (ed::AcceptNewItem()) {
					links.push_back({ GenerateLinkId(), outputId, inputId });
				}
			} else {
				ed::RejectNewItem();
			}
		} else {
			ed::RejectNewItem();
		}
	}
	ed::EndCreate();
}

void ImGuiManager::HandleDeleteLink(std::vector<Link>& links) {
	if (ed::BeginDelete()) {
		ed::LinkId deletedLinkId;
		while (ed::QueryDeletedLink(&deletedLinkId)) {
			if (ed::AcceptDeletedItem()) {
				// IDで探して削除
				links.erase(std::remove_if(links.begin(), links.end(),
					[deletedLinkId](const Link& link) {
						return link.id == deletedLinkId;
					}), links.end());
			}
		}
	}
	ed::EndDelete();
}

void ImGuiManager::CreateLink(std::vector<Link>& links, const Pin& input, const Pin& output) {
	if (CanCreateLink(output, input) && !input.isLinked) {
		links.push_back({ GenerateLinkId(), output.id, input.id });
	}
}

void ImGuiManager::DrawNode(MyNode& node, ed::Utilities::BlueprintNodeBuilder& builder) {
	builder.Begin(node.id);
	builder.Header(ImColor(128, 195, 248));

	ImGui::Text("%s", node.name.c_str());
	builder.EndHeader();

	// ---------------------------------------------------------
	// 1. Left Column: Inputs
	// ---------------------------------------------------------
	ImGui::BeginGroup();
	for (const auto& pin : node.inputs) {
		builder.Input(pin.id);

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.8f);
		ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
		ed::PinPivotSize(ImVec2(0, 0));

		if (pin.pinType == PinType::Material) {
			DrawPinIcon(pin.isLinked, ax::Widgets::IconType::Square);
		} else {
			DrawPinIcon(pin.isLinked);
		}
		ImGui::PopStyleVar();
		builder.EndInput();
	}
	ImGui::EndGroup();

	// ---------------------------------------------------------
	// 2. Middle Column: Node Contents (Widgets)
	// ---------------------------------------------------------
	ImGui::SameLine();
	ImGui::BeginGroup();

	// コンテンツがある場合のみ右横に移動、なければ余白のみ
	if (node.type != MyNode::NodeType::Texture &&
		node.type != MyNode::NodeType::Add &&
		node.type != MyNode::NodeType::Color &&
		node.type != MyNode::NodeType::Vector2) {
		// コンテンツがないノードの場合、ピン同士がくっつきすぎないように最小幅を確保
		ImGui::SameLine();
		ImGui::Dummy(ImVec2(20.0f, 0.0f));
	}

	// === Texture Node ===
	if (node.type == MyNode::NodeType::Texture) {
		ImGui::Image((ImTextureID)TextureManager::GetInstance()->GetTexture(node.texName)->gpuHandle.ptr, { 70,70 });
	}

	// === Add Node ===
	if (node.type == MyNode::NodeType::Add) {
		ImGui::Text("AddType");
		int addType = static_cast<int>(node.addType);
		if (ImGui::RadioButton("Increment", addType == static_cast<int>(AddType::Increment))) {
			addType = static_cast<int>(AddType::Increment);
		}
		// RadioButtonは改行されるのでSameLineは不要（縦に並べる場合）
		if (ImGui::RadioButton("DeltaTime", addType == static_cast<int>(AddType::DeltaTime))) {
			addType = static_cast<int>(AddType::DeltaTime);
		}
		node.addType = static_cast<AddType>(addType);

		if (node.addType == AddType::Increment) {
			ImGui::SetNextItemWidth(100.0f);
			ImGui::DragFloat(("##Add" + std::to_string(static_cast<uintptr_t>(node.id))).c_str(), &node.values[0].Get<float>(), 0.01f);
		}
	}

	// === Color Node ===
	if (node.type == MyNode::NodeType::Color) {
		ImGui::SetNextItemWidth(150.0f); // 少し幅を調整
		ImGui::ColorEdit4(("##Color" + std::to_string(static_cast<uintptr_t>(node.id))).c_str(), &node.values[0].Get<Vector4>().x);
	}

	// === Vector2 Node ===
	if (node.type == MyNode::NodeType::Vector2) {
		// ピンの高さと合わせるために少し下げるなどの微調整が必要ならDummyを入れる
		ImGui::Text("X"); ImGui::SameLine();
		ImGui::SetNextItemWidth(100.0f);
		ImGui::DragFloat(("##Vector2x" + std::to_string(static_cast<uintptr_t>(node.id))).c_str(), &node.values[0].Get<Vector2>().x, 0.01f);

		ImGui::Text("Y"); ImGui::SameLine();
		ImGui::SetNextItemWidth(100.0f);
		ImGui::DragFloat(("##Vector2y" + std::to_string(static_cast<uintptr_t>(node.id))).c_str(), &node.values[0].Get<Vector2>().y, 0.01f);
	}

	// 右側のピンとの余白
	ImGui::Dummy(ImVec2(10.0f, 0.0f));
	ImGui::EndGroup();


	// ---------------------------------------------------------
	// 3. Right Column: Outputs
	// ---------------------------------------------------------
	ImGui::SameLine();
	ImGui::BeginGroup();
	for (const auto& pin : node.outputs) {
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.8f);
		builder.Output(pin.id);

		ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
		ed::PinPivotSize(ImVec2(0, 0));

		if (pin.pinType == PinType::Material) {
			DrawPinIcon(pin.isLinked, ax::Widgets::IconType::Square);
		} else {
			DrawPinIcon(pin.isLinked);
		}
		ImGui::PopStyleVar();
		builder.EndOutput();
	}
	ImGui::EndGroup();

	builder.End();
}

void ImGuiManager::DrawNodeEditor(NodeGraph* nodeGraph) {
	ed::Begin("My Node Editor");

	ed::Utilities::BlueprintNodeBuilder builder((ImTextureID)backGroundHandle_.ptr, 126, 126);

	ed::Suspend();
	ed::NodeId nodeId;
	if (ed::ShowNodeContextMenu(&nodeId)) {
		ImGui::OpenPopup("Node Context Menu");
		selectNodeID_ = nodeId;
	} else if (ed::ShowBackgroundContextMenu()) {
		ImGui::OpenPopup("Background Context Menu");
	}
	ed::Resume();

	ed::Suspend();
	if (ImGui::BeginPopup("Node Context Menu")) {
		if (ImGui::MenuItem("Delete Node")) {
			// nodeId を削除
		}
		if (ImGui::BeginMenu("Change Texture")) {
			MyNode* selectNode = nodeGraph->FindNodeById(selectNodeID_);
			for (auto& pair : TextureManager::GetInstance()->GetTextureFiles()) {
				ImGui::Image((ImTextureID)TextureManager::GetInstance()->GetTexture(pair.first.c_str())->gpuHandle.ptr, { 30,30 });
				ImGui::SameLine();
				if (ImGui::MenuItem(pair.first.c_str())) {
					selectNode->texName = pair.first.c_str();
				}
			}
			ImGui::EndMenu();
			// nodeId に対応する Texture を変更
		}
		ImGui::EndPopup();
	}
	ed::Resume();

	ed::Suspend(); // 背景の右クリックメニュー
	if (ImGui::BeginPopup("Background Context Menu")) {
		if (ImGui::BeginMenu("Create Node")) {
			TextureSelectMenu(nodeGraph);
			if (ImGui::MenuItem("Float Node")) {
			}
			if (ImGui::MenuItem("Add Node")) {
				MyNode node;
				node.CreateNode(MyNode::NodeType::Add);
				nodeGraph->AddNode(node);
			}
			if (ImGui::MenuItem("Color Node")) {
				MyNode node;
				node.CreateNode(MyNode::NodeType::Color);
				nodeGraph->AddNode(node);
			}
			if (ImGui::MenuItem("Vector2 Node")) {
				MyNode node;
				node.CreateNode(MyNode::NodeType::Vector2);
				nodeGraph->AddNode(node);
			}
			if (ImGui::MenuItem("Material Node")) {
				MyNode node;
				node.CreateNode(MyNode::NodeType::SubMaterial);
				nodeGraph->AddNode(node);
			}
			ImGui::EndMenu();
		}
		ImGui::EndPopup();
	}
	ed::Resume();

	for (auto& node : nodeGraph->nodes) {
		DrawNode(node, builder);
	}
	for (const auto& link : nodeGraph->links) {
		ed::Link(link.id, link.startPinId, link.endPinId);
	}

	HandleCreateLink(nodeGraph->links, nodeGraph->nodes);

	HandleDeleteLink(nodeGraph->links);

	ed::End();
}

void ImGuiManager::DrawPinIcon(bool connected, ax::Widgets::IconType icon) {

	ImColor  color = ImColor(147, 226, 74);
	color.Value.w = 200.0f / 255.0f;

	ax::Widgets::Icon(ImVec2(static_cast<float>(24), static_cast<float>(24)), icon, connected, color, ImColor(32, 32, 32, 200));
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
void ImGuiManager::TextureSelectMenu(NodeGraph* nodeGraph) {
	if (ImGui::BeginMenu("Texture Node")) {
		for (auto& pair : TextureManager::GetInstance()->GetTextureFiles()) {
			ImGui::Image((ImTextureID)TextureManager::GetInstance()->GetTexture(pair.first.c_str())->gpuHandle.ptr, { 30,30 });
			ImGui::SameLine();
			if (ImGui::MenuItem(pair.first.c_str())) {
				MyNode node;
				node.CreateNode(MyNode::NodeType::Texture);
				node.values.push_back(NodeValue(pair.first.c_str()));
				node.texName = pair.first.c_str();
				nodeGraph->AddNode(node);
			}
		}
		ImGui::EndMenu();
	}
}
#endif // _DEBUG


#ifdef _DEBUGMODE
// ParticleGroup
void ParticleGroupSelector::Show(std::function<void(const std::string&, bool)> on_move) {

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
