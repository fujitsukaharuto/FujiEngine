#include "GPUParticleSystemEditor.h"
#ifdef _DEBUGMODE
#include <algorithm>
#include "Engine/Graphics/Particle/GPUParticle/GPUParticleSystem.h"
#include "Engine/Graphics/Particle/ParticleManager.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/Editor/Widget/GizmoHelper.h"

using namespace Core;
using namespace Math;
using namespace Graphics;
using namespace DXC;


namespace Editor {

	void GPUParticleSystemEditor::DrawSceneGUI(GPUParticleSystem& system) {
		DebugWindow window{ "GPUParticle Editor", ImGuiWindowFlags_NoCollapse };
		if (!window) { return; }

		DrawPerformanceStats(system);

		ImGui::Separator();

		if (ImGui::BeginTabBar("EmitterTabs")) {

			if (ImGui::BeginTabItem("Sphere Emitters")) {
				if (ImGui::Button("＋ エミッターを追加")) { system.InitGPUEmitter(); }

				DrawEmitterList(system, system.sphereEmitters_, editCSEmitInd_, PipelinePhase::Sphere);
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("TextureBased Emitters")) {
				DrawEmitterList(system, system.textureBasedEmitters_, editCSEmitTexInd_, PipelinePhase::Texture);
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Surface Emitters")) {
				DrawEmitterList(system, system.MeshSurfaceEmitters_, editCSEmitSurfaceInd_, PipelinePhase::Surface);
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
	}

	void GPUParticleSystemEditor::DrawInspectorGUI(GPUParticleSystem& system) {
		DrawPerformanceStats(system);
		DrawSphereEmitterGUI(system);
		DrawTextureEmitterGUI(system);
		DrawSurfaceEmitterGUI(system);
	}

	void GPUParticleSystemEditor::DrawPerformanceStats(GPUParticleSystem& system) {
		if (ImGui::CollapsingHeader("統計情報 / パフォーマンス")) {
			uint32_t frameIndex = system.dxcommon_->GetNowFrameCount();
			uint32_t finishedFrame = (frameIndex + DXC::kFrameCount_ - 1) % DXC::kFrameCount_;

			ImGui::Checkbox("コンピュート・スプラット描画", &system.useComputeSplat_);
			ImGui::SameLine();
			ImGui::TextDisabled("(OFF=従来ラスタ1/4解像度)");

			ImGui::Checkbox("フル解像度 (深度テスト有効)", &system.useFullResolution_);
			ImGui::SameLine();
			ImGui::TextDisabled("(OFF=1/4扱い・深度テスト無効。splat時のシーン遮蔽)");

			ImGui::Checkbox("オーバーラップ (Stage2: Update∥Draw)", &system.useOverlap_);
			ImGui::SameLine();
			ImGui::TextDisabled("(splat時のみ。OFF=従来直列でA/B比較)");

			// テーブルを使うと数値が揃って見やすくなります
			if (ImGui::BeginTable("StatsTable", 2, ImGuiTableFlags_BordersInnerV)) {

				system.AliveCountDataReadBack();

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0); ImGui::Text("生存パーティクル数:");
				ImGui::TableSetColumnIndex(1); ImGui::Text("%d", system.aliveCount_);

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0); ImGui::Text("描画負荷 (Draw):");
				ImGui::TableSetColumnIndex(1); ImGui::Text("%.3f ms", system.gpuTimerGraphics.GetElapsedMS(finishedFrame, GPUParticleSystem::kTimer_DrawExecuteIndirect));

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0); ImGui::Text("更新負荷 (Update):");
				ImGui::TableSetColumnIndex(1); ImGui::Text("%.3f ms", system.gpuTimerCompute.GetElapsedMS(finishedFrame, GPUParticleSystem::kTimer_ParticleUpdate));

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0); ImGui::Text("発生負荷 (Emit):");
				ImGui::TableSetColumnIndex(1); ImGui::Text("%.3f ms", system.gpuTimerCompute.GetElapsedMS(finishedFrame, GPUParticleSystem::kTimer_EmitterDispatch));

				ImGui::EndTable();
			}
		}
	}

	void GPUParticleSystemEditor::DrawEmitterList(GPUParticleSystem& system, std::vector<int>& emitterIndices, int& currentIdx, PipelinePhase phase) {
		if (emitterIndices.empty()) {
			ImGui::TextDisabled("エミッターが存在しません。");
			return;
		}
		ImGui::Checkbox("Tracking", &isMouseTracking_);

		if (currentIdx >= emitterIndices.size()) {
			currentIdx = int(emitterIndices.size()) - 1;
		}

		// 左側にリスト、右側に詳細を表示する
		ImGui::BeginChild("ListRegion", ImVec2(150, 0), ImGuiChildFlags_Border);
		for (int n = 0; n < (int)emitterIndices.size(); n++) {
			ImGui::PushID(n);
			if (ImGui::Selectable("##selectable", currentIdx == n, ImGuiSelectableFlags_SpanAllColumns)) {
				currentIdx = n;
			}
			ImGui::SameLine();
			ImGui::Text("エミッター %d", n);
			ImGui::PopID();
		}
		ImGui::EndChild();

		ImGui::SameLine();

		// 右側の詳細設定
		ImGui::BeginGroup();
		ImGui::BeginChild("DetailRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Border);

		int actualIdx = emitterIndices[currentIdx];
		ImGui::Text("詳細設定 - ID: %d", actualIdx);
		ImGui::Separator();

		// エミッター固有のGUIを呼び出し
		system.csEmitters_[actualIdx].emitter->DebugGUI();
		DrawLoadPopUp(system, currentIdx, phase);

		if (isMouseTracking_) {
			DrawMouseTransGizmo();
			system.csEmitters_[actualIdx].emitter->SetPos(mouseTrans_.translate);
		}

		ImGui::EndChild();
		ImGui::EndGroup();
	}

	void GPUParticleSystemEditor::DrawSphereEmitterGUI(GPUParticleSystem& system) {
		if (ImGui::CollapsingHeader("GPU Particle Emitter")) {
			if (ImGui::Button("Emitterの追加")) {
				system.InitGPUEmitter();
			}
			if (system.csEmitters_.size() == 0 || system.sphereEmitters_.size() == 0) return;
			if (ImGui::ArrowButton("Index-", ImGuiDir_Left)) {
				if (editCSEmitInd_ > 0) {
					editCSEmitInd_--;
				}
			}ImGui::SameLine();
			ImGui::DragInt("##emitIndex", &editCSEmitInd_, 1.0f, 0, int(system.sphereEmitters_.size() - 1), "EmitIndex : %d");
			ImGui::SameLine();
			if (ImGui::ArrowButton("Index+", ImGuiDir_Right)) {
				if (editCSEmitInd_ < int(system.sphereEmitters_.size() - 1)) {
					editCSEmitInd_++;
				}
			}
			// 括弧で囲むのは Windows.h の min マクロに展開させないため
			int idx = (std::min)(editCSEmitInd_, static_cast<int>(system.sphereEmitters_.size()) - 1);
			editCSEmitInd_ = idx;
			if (ImGui::TreeNode("ParticleCS Emit Control")) {
				system.csEmitters_[system.sphereEmitters_[idx]].emitter->DebugGUI();
				DrawLoadPopUp(system, idx, PipelinePhase::Sphere);
				ImGui::TreePop();
			}
		}
	}

	void GPUParticleSystemEditor::DrawTextureEmitterGUI(GPUParticleSystem& system) {
		if (system.csEmitters_.size() == 0 || system.textureBasedEmitters_.size() == 0) return;
		if (ImGui::CollapsingHeader("GPU ParticleTex Emitter")) {
			if (ImGui::ArrowButton("Index-", ImGuiDir_Left)) {
				if (editCSEmitTexInd_ > 0) {
					editCSEmitTexInd_--;
				}
			}ImGui::SameLine();
			ImGui::DragInt("##emitIndex", &editCSEmitTexInd_, 1.0f, 0, int(system.textureBasedEmitters_.size() - 1), "EmitIndex : %d");
			ImGui::SameLine();
			if (ImGui::ArrowButton("Index+", ImGuiDir_Right)) {
				if (editCSEmitTexInd_ < int(system.textureBasedEmitters_.size() - 1)) {
					editCSEmitTexInd_++;
				}
			}
			int idx = (std::min)(editCSEmitTexInd_, static_cast<int>(system.textureBasedEmitters_.size()) - 1);
			editCSEmitTexInd_ = idx;
			system.csEmitters_[system.textureBasedEmitters_[idx]].emitter->DebugGUI();
		}
	}

	void GPUParticleSystemEditor::DrawSurfaceEmitterGUI(GPUParticleSystem& system) {
		if (system.csEmitters_.size() == 0 || system.MeshSurfaceEmitters_.size() == 0) return;
		if (ImGui::CollapsingHeader("GPU Particle Surface Emitter")) {
			if (ImGui::ArrowButton("Index-", ImGuiDir_Left)) {
				if (editCSEmitSurfaceInd_ > 0) {
					editCSEmitSurfaceInd_--;
				}
			}ImGui::SameLine();
			ImGui::DragInt("##emitIndex", &editCSEmitSurfaceInd_, 1.0f, 0, int(system.MeshSurfaceEmitters_.size() - 1), "EmitIndex : %d"); ImGui::SameLine();
			if (ImGui::ArrowButton("Index+", ImGuiDir_Right)) {
				if (editCSEmitSurfaceInd_ < int(system.MeshSurfaceEmitters_.size() - 1)) {
					editCSEmitSurfaceInd_++;
				}
			}

			int idx = (std::min)(editCSEmitSurfaceInd_, static_cast<int>(system.MeshSurfaceEmitters_.size()) - 1);
			editCSEmitSurfaceInd_ = idx;
			if (ImGui::TreeNode("ParticleCS Emit Control")) {
				system.csEmitters_[system.MeshSurfaceEmitters_[idx]].emitter->DebugGUI();
				DrawLoadPopUp(system, idx, PipelinePhase::Surface);
				ImGui::TreePop();
			}
		}
	}

	void GPUParticleSystemEditor::DrawLoadPopUp(GPUParticleSystem& system, int id, PipelinePhase type) {
		if (ImGui::Button("LoadFile")) {
			ImGui::OpenPopup("CSEmitterFile Window");
		}

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		// 少し大きめのウィンドウサイズを指定（必要に応じて調整してください）
		ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);

		if (ImGui::BeginPopupModal("CSEmitterFile Window", NULL)) {
			// --- 上部：ファイル一覧エリア（スクロール可能にする） ---
			// 下部のボタンエリア(約40px)を残して残りを一覧表示に使う
			if (ImGui::BeginChild("FileScrollingRegion", ImVec2(0, -40), true, ImGuiWindowFlags_HorizontalScrollbar)) {
				float windowVisibleX = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
				ImVec2 buttonSize(100, 100); // ボタンのサイズ
				ImGuiStyle& style = ImGui::GetStyle();

				auto& fileNames = ParticleManager::GetInstance()->GetCSEmitterFileNames();
				for (size_t i = 0; i < fileNames.size(); i++) {
					const auto& filename = fileNames[i];

					// ボタンを描画（ID衝突回避のため ##i を付与するか、PushIDを使う）
					ImGui::PushID((int)i);
					if (ImGui::Button(filename.c_str(), buttonSize)) {
						switch (type) {
						case PipelinePhase::Texture:
							system.csEmitters_[system.textureBasedEmitters_[id]].emitter->Load(filename);
							break;
						case PipelinePhase::Surface:
							system.csEmitters_[system.MeshSurfaceEmitters_[id]].emitter->Load(filename);
							break;
						case PipelinePhase::Sphere:
							system.csEmitters_[system.sphereEmitters_[id]].emitter->Load(filename);
							break;
						default:
							break;
						}
						// 選択したらウィンドウを閉じる
						ImGui::CloseCurrentPopup();
					}
					ImGui::PopID();

					float lastButtonX = ImGui::GetItemRectMax().x;
					float nextButtonX = lastButtonX + style.ItemSpacing.x + buttonSize.x;
					// 次のボタンがウィンドウ端を越えない、かつ リストの最後でなければ SameLine
					if (i + 1 < fileNames.size() && nextButtonX < windowVisibleX) {
						ImGui::SameLine();
					}
				}
				ImGui::EndChild();
			}

			// --- 下部：操作ボタンエリア ---
			ImGui::Separator();

			float buttonWidth = 120.0f;
			float spaceWidth = ImGui::GetStyle().ItemSpacing.x;
			float totalButtonWidth = (buttonWidth * 2) + spaceWidth;
			ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - totalButtonWidth);
			if (ImGui::Button("OK", ImVec2(buttonWidth, 0))) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0))) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}

	void GPUParticleSystemEditor::DrawMouseTransGizmo() {
		GizmoHelper::ManipulateSimple(mouseTrans_, GizmoOperation::Translate);
	}
}
#endif // _DEBUGMODE
