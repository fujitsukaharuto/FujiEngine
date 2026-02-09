#include "Boss.h"
#include "Game/GameObj/Enemy/Behavior/BossRoot.h"
#include "Game/GameObj/Enemy/Behavior/BossAttack.h"
#include "Game/GameObj/Enemy/Behavior/BossJumpAttack.h"
#include "Game/GameObj/Enemy/Behavior/BossSwordAttack.h"
#include "Game/GameObj/Enemy/Behavior/BossBeamAttack.h"
#include "Game/GameObj/Enemy/Behavior/BossAreaAttack.h"
#include "Game/GameObj/Enemy/Behavior/BossArrowAttack.h"
#include "Game/GameObj/Enemy/Behavior/BossRodFall.h"
#include "Game/GameObj/Enemy/Behavior/BossDushAttack.h"


void Boss::DebugGUI() {
#ifdef _DEBUG
	if (ImGui::CollapsingHeader("Boss##0")) {
		ImGui::Indent();
		if (ImGui::CollapsingHeader("Boss##1")) {
			animeModel_->DebugGUI();
			collider_->DebugGUI();
			ParameterGUI();
		}
		core_->DebugGUI();
		ImGui::Unindent();
	}
#endif // _DEBUG
}


void Boss::ParameterGUI() {
#ifdef _DEBUG
	ImGui::Indent();
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Selected;
	if (ImGui::TreeNodeEx("BossBehavior", flags)) {
		static int currentActionIndex = 0;
		static std::string nowAction = actionList_[currentActionIndex];
		// コンボボックスの表示に使うため、const char* の配列に変換
		std::vector<const char*> actionCStrs;
		for (const auto& str : actionList_) {
			actionCStrs.push_back(str.c_str());
		}

		if (ImGui::Combo("Action", &currentActionIndex, actionCStrs.data(), static_cast<int>(actionCStrs.size()))) {
			nowAction = actionList_[currentActionIndex];
		}
		if (ImGui::Button("SetAction")) {
			SetDefaultBehavior(true);
			static const std::unordered_map<std::string, std::function<std::unique_ptr<BaseBossBehavior>(Boss*)>> behaviorFactory = {
				{ "Root",   [](Boss* b) { return std::make_unique<BossRoot>(b); } },
				{ "Wave", [](Boss* b) { return std::make_unique<BossAttack>(b); } },
				{ "Beam",   [](Boss* b) { return std::make_unique<BossBeamAttack>(b); } },
				{ "Jump",   [](Boss* b) { return std::make_unique<BossJumpAttack>(b); } },
				{ "Sword",   [](Boss* b) { return std::make_unique<BossSwordAttack>(b); } },
				{ "Area",   [](Boss* b) { return std::make_unique<BossAreaAttack>(b); } },
				{ "Arrow",   [](Boss* b) { return std::make_unique<BossArrowAttack>(b); } },
				{ "FallRod",   [](Boss* b) { return std::make_unique<BossRodFall>(b); } },
				{ "Dash",   [](Boss* b) { return std::make_unique<BossDushAttack>(b); } },
				// 他も追加
			};
			auto it = behaviorFactory.find(nowAction);
			if (it != behaviorFactory.end()) {
				ChangeBehavior(it->second(this));
			} else {
				ImGui::Text("Unknown Action: %s", nowAction.c_str());
			}
		}
		if (nowAction != "Root") {
			ImGui::SameLine();
			if (ImGui::Button("AddAction")) {
				phaseList_[phaseIndex_].push_back({ nowAction, 1.0f });
			}
		}


		int phase = int(phaseList_.size());
		if (phase != 0) {
			if (ImGui::BeginTable("table_Phase", phase, ImGuiTableFlags_Borders)) {
				for (int i = 1; i < phase + 1; i++) {
					ImGui::TableSetupColumn(("Phase" + std::to_string(i)).c_str());
				}
				ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
				for (int column = 0; column < phase; column++) {
					ImGui::TableSetColumnIndex(column);
					const char* column_name = ImGui::TableGetColumnName(column); // Retrieve name passed to TableSetupColumn()
					ImGui::PushID(column);
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
					ImGui::RadioButton("##checkAll", &phaseIndex_, column);
					ImGui::PopStyleVar();
					ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
					ImGui::TableHeader(column_name);
					ImGui::PopID();
				}

				ImGui::TableNextRow();
				for (int phaseCount = 0; phaseCount < phase; phaseCount++) {
					ImGui::TableSetColumnIndex(phaseCount);
					int tableCount = 2;
					if (phaseList_[phaseCount].size() > 1) tableCount++;
					if (ImGui::BeginTable("table_ActionList", tableCount, ImGuiTableFlags_Borders)) {
						ImGui::TableSetupColumn("Action");
						ImGui::TableSetupColumn("Weight");
						if (tableCount == 3) ImGui::TableSetupColumn("Delete");
						ImGui::TableHeadersRow();

						for (int row = 0; row < phaseList_[phaseCount].size(); row++) {
							ImGui::TableNextRow();
							if (row == 0) {
								ImGui::TableSetColumnIndex(1);
								ImGui::PushItemWidth(50.0f);
								if (tableCount == 3) {
									ImGui::TableSetColumnIndex(2);
									ImGui::PushItemWidth(50.0f);
								}
							}

							// Draw our contents
							ImGui::PushID(row);
							ImGui::TableSetColumnIndex(0);
							ImGui::Text(phaseList_[phaseCount][row].first.c_str());
							ImGui::TableSetColumnIndex(1);
							ImGui::SliderFloat(("##" + phaseList_[phaseCount][row].first + std::to_string(phaseCount)).c_str(), &phaseList_[phaseCount][row].second, 0.0f, 1.0f);
							if (tableCount == 3) {
								ImGui::TableSetColumnIndex(2);
								if (ImGui::Button(("X##del" + phaseList_[phaseCount][row].first + std::to_string(phaseCount)).c_str())) {
									phaseList_[phaseCount].erase(phaseList_[phaseCount].begin() + row);
								}
							}
							ImGui::PopID();
						}
						ImGui::EndTable();
					}
				}

				ImGui::EndTable();
			}
		}
		if (ImGui::Button(("save##phase"))) {
			SavePhase();
		}ImGui::SameLine();
		if (ImGui::Button(("load##phase"))) {
			LoadPhase();
		}

		ImGui::TreePop();
	}
	ImGui::Unindent();
#endif // _DEBUG
}