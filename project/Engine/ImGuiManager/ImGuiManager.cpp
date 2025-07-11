#include "ImGuiManager.h"

#include "DXCom.h"
#include "MyWindow.h"
#include "SRVManager.h"
#include "Engine/Input/Input.h"
#include "Engine/Editor/JsonSerializer.h"
#include "Engine/Model/TextureManager.h"
#ifdef _DEBUG
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#include "ImGuizmo.h"
#include "imgui_node_editor.h"
#include "externals/imgui/utilities/widgets.h"

namespace ed = ax::NodeEditor;
#endif // _DEBUG

ImGuiManager* ImGuiManager::GetInstance() {
	static ImGuiManager instance;
	return &instance;
}

void ImGuiManager::Initialize([[maybe_unused]] MyWin* myWin, [[maybe_unused]] DXCom* dxComon) {
	dxCommon_ = dxComon;
#ifdef _DEBUG

	SRVManager* srvManager = SRVManager::GetInstance();
	ID3D12DescriptorHeap* srv = srvManager->GetSRVHeap();
	srvIndex = srvManager->Allocate();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	/// style
	ImGui::GetStyle().FrameRounding = 4;
	ImGui::GetStyle().TabRounding = 8;
	ImGui::GetStyle().Colors[33] = { 0.55f,0.588f,0.632f,0.8f };
	ImGui::GetStyle().Colors[34] = { 0.091f,0.179f,0.299f,0.862f };
	ImGui::GetStyle().Colors[35] = { 0.109f,0.522f,0.0f,1.0f };
	ImGui::GetStyle().Colors[ImGuiCol_ModalWindowDimBg] = { 0.8f,0.8f,0.8f,0.075f };

	ImGui::SetColorEditOptions(ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueBar);

	/// font
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF("imgui/FiraMono-Medium.ttf", 16.0f);

	ImFontConfig font_config;
	font_config.SizePixels = 18.0f;
	font_japanese = io.Fonts->AddFontFromFileTTF("imgui/NotoSansJP-Regular.ttf", 18.0f, &font_config, io.Fonts->GetGlyphRangesJapanese());


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

	winSizeX_ = myWin->kWindowWidth;
	winSizeY_ = myWin->kWindowHeight;
#endif // _DEBUG
}

void ImGuiManager::Fin() {
	dxCommon_ = nullptr;
#ifdef _DEBUG
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
#ifdef _DEBUG
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGuizmo::BeginFrame();
	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetRect(0, 0, winSizeX_, winSizeY_);

#endif // _DEBUG
}

void ImGuiManager::End() {
#ifdef _DEBUG
	ImGui::Render();
#endif // _DEBUG
}

void ImGuiManager::Draw() {
	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

#ifdef _DEBUG
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
#endif // _DEBUG
}

void ImGuiManager::SetFontJapanese() {
#ifdef _DEBUG
	ImGui::PushFont(font_japanese);
#endif // _DEBUG
}

void ImGuiManager::UnSetFont() {
#ifdef _DEBUG
	ImGui::PopFont();
#endif // _DEBUG
}

#ifdef _DEBUG
void ImGuiManager::ImGuiDragDropButton(const char* buttonLabel, const char* payloadType, const std::function<void(const ImGuiPayload* payload)>& onDrop, const std::function<void(const ImGuiPayload* payload)>& onPreview) {
#ifdef _DEBUG
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
#ifdef _DEBUG
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
#endif // _DEBUG

#ifdef _DEBUG
void ImGuiManager::InitNodeTexture() {
	backGroundHandle_ = TextureManager::GetInstance()->GetTexture("BlueprintBackground.png")->gpuHandle;
}
#endif // _DEBUG

//void ImGuiManager::DrawNodeEditor() {
//#ifdef _DEBUG
//	using namespace ax::NodeEditor;
//
//	if (!nodeEditorContext_) {
//		Config config;
//		config.SettingsFile = "resource/NodeEditor/NodeEditor.json";
//		nodeEditorContext_ = CreateEditor(&config);
//	}
//
//	SetCurrentEditor(nodeEditorContext_);
//	ed::Begin("My Node Editor");
//
//	// 簡単なノード描画（省略）
//
//	ed::End();
//	SetCurrentEditor(nullptr);
//#endif // _DEBUG
//}


#ifdef _DEBUG
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

			if (inPin && outPin && CanCreateLink(*outPin, *inPin) && !inPin->isLinked) {
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

void ImGuiManager::DrawNode(MyNode& node, ed::Utilities::BlueprintNodeBuilder& builder) {
	builder.Begin(node.id);
	builder.Header(ImColor(128, 195, 248));

	ImGui::Text("%s", node.name.c_str());
	builder.EndHeader();

	ImGui::BeginGroup();
	for (const auto& pin : node.inputs) {
		builder.Input(pin.id);

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.8f);
		ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
		ed::PinPivotSize(ImVec2(0, 0));

		DrawPinIcon(pin.isLinked);
		ImGui::PopStyleVar();
		builder.EndInput();
	}

	ImGui::EndGroup();
	ImGui::SameLine(70);

	for (const auto& pin : node.outputs) {
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.8f);
		builder.Output(pin.id);

		ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
		ed::PinPivotSize(ImVec2(0, 0));

		DrawPinIcon(pin.isLinked);
		ImGui::PopStyleVar();
		builder.EndOutput();
	}

	if (node.type == MyNode::NodeType::Texture) {
		ImGui::Dummy(ImVec2(5.0f, 0.0f)); ImGui::SameLine();
		ImGui::Image((ImTextureID)TextureManager::GetInstance()->GetTexture(node.texName)->gpuHandle.ptr, { 70,70 });
	}

	if (node.type == MyNode::NodeType::Color) {
		ImGui::Dummy(ImVec2(5.0f, 0.0f)); ImGui::SameLine();
		ImGui::ColorEdit4(("##Color" + std::to_string(static_cast<uintptr_t>(node.id))).c_str(), &node.values[0].Get<Vector4>().x);
		/*ed::Suspend();

		ed::Resume();*/
	}

	builder.End();
}

void ImGuiManager::DrawNodeEditor(NodeGraph* nodeGraph) {
	ed::Begin("My Node Editor");

	ed::Utilities::BlueprintNodeBuilder builder((ImTextureID)backGroundHandle_.ptr, 126, 126);

	ed::Suspend();
	ed::NodeId nodeId;
	if (ed::ShowNodeContextMenu(&nodeId)) {
		ImGui::OpenPopup("Node Context Menu");
	} else if (ed::ShowBackgroundContextMenu()) {
		ImGui::OpenPopup("Background Context Menu");
	}
	ed::Resume();

	ed::Suspend(); // ノードの右クリックメニュー
	if (ImGui::BeginPopup("Node Context Menu")) {
		if (ImGui::MenuItem("Delete Node")) {
			// nodeId を削除
		}
		if (ImGui::MenuItem("Change Texture")) {
			// nodeId に対応する Texture を変更
		}
		ImGui::EndPopup();
	}
	ed::Resume();

	ed::Suspend(); // 背景の右クリックメニュー
	if (ImGui::BeginPopup("Background Context Menu")) {
		if (ImGui::BeginMenu("Add Node")) {
			if (ImGui::MenuItem("Blueprint Node")) {
				// Blueprintノード追加処理
			}
			if (ImGui::MenuItem("Texture Node")) {
				// Textureノード追加処理
			}
			if (ImGui::MenuItem("Float Node")) {
				// Floatノード追加処理
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

void ImGuiManager::DrawPinIcon(bool connected) {

	ax::Widgets::IconType iconType;
	ImColor  color = ImColor(147, 226, 74);
	color.Value.w = 200.0f / 255.0f;
	iconType = ax::Widgets::IconType::Circle;

	ax::Widgets::Icon(ImVec2(static_cast<float>(24), static_cast<float>(24)), iconType, connected, color, ImColor(32, 32, 32, 200));
}

#endif // _DEBUG


#ifdef _DEBUG
// ParticleGroup
void ParticleGroupSelector::Show(std::function<void(const std::string&, bool)> on_move) {

	const float listBoxHeight = 200.0f;

	if (ImGui::BeginTable("ParticleGroupTable", 3, ImGuiTableFlags_None)) {
		ImGui::TableSetupColumn("Emitte: False", ImGuiTableColumnFlags_WidthStretch);    // Left side
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);      // Buttons
		ImGui::TableSetupColumn("Emitte: True", ImGuiTableColumnFlags_WidthStretch);    // Right side
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
			//// 中央揃えするためにダミースペースを使う
			//float buttonHeight = ImGui::GetFrameHeight();
			//float spacing = (listBoxHeight - buttonHeight * 2.0f - ImGui::GetStyle().ItemSpacing.y) * 0.5f;
			//ImGui::Dummy(ImVec2(0.0f, spacing));

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
