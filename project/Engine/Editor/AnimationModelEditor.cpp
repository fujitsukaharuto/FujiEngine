#include "AnimationModelEditor.h"
#ifdef _DEBUGMODE
#include "Engine/Model/AnimationData/AnimationModel.h"
#include "Engine/Model/Line/Line3dDrawer.h"
#include "Engine/ImGuiManager/ImGuiManager.h"

using namespace Graphics;
using namespace Math;


namespace Editor {

	void AnimationModelEditor::DrawGUI(AnimationModel& model) {
		ImGui::Indent();
		if (ImGui::DragFloat("EnvironmentCoeff", &model.environmentCoeff_, 0.01f, 0.0f, 1.0f)) {
			for (Material& material : model.material_) {
				material.SetEnvironment(model.environmentCoeff_);
			}
		}
		Vector4 color = model.material_[0].GetColor();
		ImGui::ColorEdit3("color", &color.x);
		model.SetColor(color);
		if (model.animations_.size() == 0) {
			ImGui::Unindent();
			return;
		}
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Selected;
		if (ImGui::TreeNodeEx("animation", flags)) {

			int currentIndex = -1;
			std::vector<const char*> animationNames;
			animationNames.reserve(model.animations_.size());

			int index = 0;
			for (const auto& [name, anime] : model.animations_) {
				animationNames.push_back(name.c_str());
				if (name == model.nowAnimationName_) {
					currentIndex = index;
				}
				++index;
			}

			if (currentIndex == -1 && !animationNames.empty()) {
				currentIndex = 0;
				model.nowAnimationName_ = animationNames[0]; // デフォルトで先頭
			}

			if (!animationNames.empty()) {
				if (ImGui::Combo("Animation Name", &currentIndex, animationNames.data(), static_cast<int>(animationNames.size()))) {
					model.ChangeAnimation(animationNames[currentIndex]);
				}
			} else {
				ImGui::Text("No animations available");
			}

			ImGui::DragFloat("Animation Time", &model.animationTime_, 0.01f, 0.0f, 100.0f);

			ImGui::TreePop();
		}
		if (ImGui::TreeNodeEx("joint", flags)) {
			ImGui::Checkbox("Draw Skeleton", &isSkeletonVisible_);
			for (int i = 0; i < model.skeleton_.joints.size(); ++i) {
				const std::string& jointName = model.skeleton_.joints[i].name;
				bool isSelected = (i == selectedJointIndex_);
				if (ImGui::Selectable(jointName.c_str(), isSelected)) {
					selectedJointIndex_ = i;
				}
			}
			ImGui::TreePop();
		}
		ImGui::Unindent();
	}

	void AnimationModelEditor::DrawSkeleton(AnimationModel& model) {
		if (!isSkeletonVisible_) {
			return;
		}
		for (size_t i = 0; i < model.skeleton_.joints.size(); ++i) {
			Joint& joint = model.skeleton_.joints[i];
			Matrix4x4 worldJointPos = Multiply(joint.skeletonSpaceMatrix, model.GetWorldMat());
			Vector3 jointPos = {
				worldJointPos.m[3][0],
				worldJointPos.m[3][1],
				worldJointPos.m[3][2]
			};

			// ハイライトの色判定
			Vector4 jointColor = (static_cast<int>(i) == selectedJointIndex_)
				? Vector4{ 1.0f, 0.0f, 0.0f, 1.0f }   // 選択中は赤で強調
				: Vector4{ 0.3f, 1.0f, 0.3f, 1.0f };  // 通常
			Line3dDrawer::GetInstance()->DrawSphereLine(jointPos, 0.05f, jointColor);

			// 親子関係のライン
			if (joint.parent) {
				Matrix4x4 worldParentJointPos = Multiply(model.skeleton_.joints[*joint.parent].skeletonSpaceMatrix, model.GetWorldMat());
				Vector3 parentPos = {
					worldParentJointPos.m[3][0],
					worldParentJointPos.m[3][1],
					worldParentJointPos.m[3][2]
				};
				Line3dDrawer::GetInstance()->DrawLine3d(jointPos, parentPos, { 1.0f,1.0f,1.0f,1.0f });
			}
		}
	}
}
#endif // _DEBUGMODE
