#include "ParticleManager.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/SRVManager.h"
#include "Engine/Camera/CameraManager.h"
#include "Particle.h"
#include "Engine/Math/Random/Random.h"
#include "Engine/DX/FPSKeeper.h"
#include "Engine/Model/ModelManager.h"
#include "Engine/ImGuiManager/ImGuiManager.h"

using namespace Core;
using namespace Graphics;
using namespace Math;
using namespace DXC;



void ParticleManager::ParticleDebugGUI() {
#ifdef _DEBUGMODE
	ImGui::Begin("Particle Editor", nullptr, ImGuiWindowFlags_NoCollapse);

	// テーブルフラグ設定：
	// Resizable: 境界線をドラッグ可能にする
	// BordersInnerV: 列の間に線（リサイズハンドル）を表示
	static float leftPaneWidth = 260.0f;
	ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV;

	if (ImGui::BeginTable("EditorMainSplit", 2, tableFlags)) {
		// --- カラム設定 ---
		// 左カラム: 初期幅固定
		ImGui::TableSetupColumn("Hierarchy", ImGuiTableColumnFlags_WidthFixed, leftPaneWidth);
		// 右カラム: 残りの領域すべて
		ImGui::TableSetupColumn("Inspector", ImGuiTableColumnFlags_WidthStretch);

		// =============================================
		// [左側] Hierarchy Pane
		// =============================================
		ImGui::TableNextColumn();

		ImGui::BeginChild("LeftPane", ImVec2(-FLT_MIN, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
		{
			ImGui::SeparatorText("Particle Groups");
			if (ImGui::BeginListBox("##GroupList", ImVec2(-FLT_MIN, -120))) {
				int idx = 0;
				for (const auto& pair : particleGroups_) {
					// 選択状態の判定
					const bool is_selected = (selectParticleGroup_ == pair.second.get());

					if (ImGui::Selectable(pair.first.c_str(), is_selected)) {
						selectParticleGroup_ = pair.second.get();
						currentKey_ = pair.first;
						currentIndex_ = idx;
					}

					// 初期フォーカス設定
					if (is_selected) {
						ImGui::SetItemDefaultFocus();
					}
					idx++;
				}
				ImGui::EndListBox();
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// グローバル操作エリア
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Global Controls");
			if (ImGui::Button("Reset All Timers", ImVec2(-FLT_MIN, 0))) {
				for (auto& groupPair : particleGroups_) {
					groupPair.second->emitter_.TimeReset();
				}
			}
			if (ImGui::Button(isStopped_ ? "Start" : "Stop", ImVec2(80, 0))) {
				isStopped_ ? FPSKeeper::SetUnStopped() : FPSKeeper::SetStopped();
				isStopped_ = !isStopped_;
			}

			ImGui::SameLine();
			// コマ送り（>>）
			if (FPSKeeper::GetFrameByFrame()) {
				FPSKeeper::SetFrameByFrame(false);
			}
			if (ImGui::Button(">>", ImVec2(40, 0))) {
				FPSKeeper::SetFrameByFrame(true);
			}

			ImGui::Spacing();

			// Emit ON/OFF ツリー
			if (ImGui::TreeNodeEx("Emit Toggle Tree", ImGuiTreeNodeFlags_Framed)) {
				static ParticleGroupSelector selector;

				if (selector.items[0].empty() && selector.items[1].empty()) {
					for (const auto& [name, group] : particleGroups_) {
						if (group->emitter_.isEmit_)
							selector.items[1].push_back(name);
						else
							selector.items[0].push_back(name);
					}
				}

				selector.Show([&](const std::string& name, bool emit) {
					auto it = particleGroups_.find(name);
					if (it != particleGroups_.end()) {
						it->second->emitter_.isEmit_ = emit;
					}
					});

				ImGui::TreePop();
			}
		}
		ImGui::EndChild(); // LeftPane End


		// =============================================
		// [右側] Inspector Pane
		// =============================================
		ImGui::TableNextColumn();

		// 右側も独立してスクロール可能にする
		ImGui::BeginChild("RightPane", ImVec2(-FLT_MIN, 0), false);
		{
			if (selectParticleGroup_) {
				ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", currentKey_.c_str());
				ImGui::Separator();
				ImGui::Spacing();

				// 横並びレイアウトでスペース効率化
				ImGui::BeginGroup();
				{
					// 画像表示
					ImGui::Image(
						(ImTextureID)TextureManager::GetInstance()->GetTexture(
							selectParticleGroup_->GetMaterial().GetPathName().c_str())->gpuHandle.ptr,
						ImVec2(80, 80),
						ImVec2(0, 0), ImVec2(1, 1),
						ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.5f) // 枠線をつける
					);
				}
				ImGui::EndGroup();
				ImGui::SameLine();
				ImGui::BeginGroup();
				{
					ImGui::Text("Material:");
					ParticleTexturePopUp();
					ImGui::Spacing();
					ImGui::TextDisabled("Path: %s", selectParticleGroup_->GetMaterial().GetPathName().c_str());
				}
				ImGui::EndGroup();

				ImGui::Spacing();
				ImGui::Separator();

				if (ImGui::CollapsingHeader("Basic Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
					ImGui::Indent();

					// Shape Type
					int shapeType = static_cast<int>(selectParticleGroup_->GetShapeType());
					ImGui::Text("形状");
					ImGui::SetNextItemWidth(-FLT_MIN);
					if (ImGui::Combo("##ShapeType", &shapeType,
						"平面\0リング\0球\0トーラス\0円柱\0コーン\0三角形\0キューブ\0雷\0")) {
						selectParticleGroup_->GetShapeType() = static_cast<ShapeType>(shapeType);
					}

					ImGui::Spacing();

					// Blend Mode
					int blendType = static_cast<int>(selectParticleGroup_->type_);
					ImGui::Text("Blend Mode");
					ImGui::SetNextItemWidth(-FLT_MIN);
					if (ImGui::Combo("##BlendType", &blendType,
						"アルファ\0加算\0減算\0スクリーン\0乗算\0ソフト加算\0プリマルチプライド\0")) {
						selectParticleGroup_->type_ = static_cast<BlendType>(blendType);
					}

					ImGui::Spacing();

					// Counts
					int maxCount = static_cast<uint32_t>(selectParticleGroup_->GetInstanceCount());
					ImGui::Text("最大パーティクル数");
					ImGui::SetNextItemWidth(-FLT_MIN);
					if (ImGui::DragInt("##MaxCount", &maxCount, 10, 1, 10000)) {
						selectParticleGroup_->GetInstanceCount() = static_cast<uint32_t>(maxCount);
					}

					ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "生存数: %d", int(selectParticleGroup_->GetDrawCount()));

					selectParticleGroup_->emitter_.EmitProgressGUI();

					ImGui::Spacing();
					if (ImGui::Button("Save Group Config", ImVec2(-FLT_MIN, 0))) {
						SaveGroupData();
					}

					ImGui::Unindent();
				}

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				selectParticleGroup_->emitter_.DebugGUI();
				ImGui::Dummy(ImVec2(0, 50));
			} else {
				// 非選択時の表示
				float winWidth = ImGui::GetContentRegionAvail().x;
				float winHeight = ImGui::GetContentRegionAvail().y;
				ImGui::SetCursorPos(ImVec2(winWidth * 0.5f - 60, winHeight * 0.5f));
				ImGui::TextDisabled("Select a Particle Group");
			}
		}
		ImGui::EndChild(); // RightPane End

		ImGui::EndTable();
	}


	ImGui::End();
#endif // _DEBUG
}


void ParticleManager::ParticleCSDebugGUI() {
#ifdef _DEBUGMODE
	if (ImGui::CollapsingHeader("GPU Particle System")) {
		ImGui::Indent();
		gpuParticleSystem_->EmitterInspectorGUI();
		ImGui::Unindent();
	}
#endif // _DEBUG
}

void ParticleManager::GPUParticleSceneGUI() {
#ifdef _DEBUGMODE
	gpuParticleSystem_->DebugGUI();
#endif // _DEBUG
}

void ParticleManager::ParticleTexturePopUp() {
#ifdef _DEBUGMODE
	if (ImGui::Button("TextureFile")) {
		ImGui::OpenPopup("TextureFile Window");
	}
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0, 0, 0, 0.25f));
	if (ImGui::BeginPopupModal("TextureFile Window", NULL)) {
		if (ImGui::Button("Refresh")) {
			TextureManager::GetInstance()->LoadTextureFile(true);
		}
		int buttonCount = 0;
		for (const auto& TexName : TextureManager::GetInstance()->GetTextureFiles()) {
			if (buttonCount > 0 && buttonCount < 5) {
				ImGui::SameLine();
			} else {
				buttonCount = 0;
			}
			if (ImGui::ImageButton(("##" + TexName.first).c_str(), (ImTextureID)TextureManager::GetInstance()->GetTexture(TexName.first.c_str())->gpuHandle.ptr, ImVec2(100, 100))) {
				selectParticleGroup_->GetMaterial().SetTextureNamePath(TexName.first.c_str());
				selectParticleGroup_->GetMaterial().SetTexture(TexName.first.c_str(), TexName.second);
				if (TexName.second) {
					TextureManager::GetInstance()->SetTextureFileOnceLoad(TexName.first.c_str());
				}
			}
			buttonCount++;
		}
		ImGui::Separator();
		if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::EndPopup();
	}
	ImGui::PopStyleColor();
#endif // _DEBUG
}