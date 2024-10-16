#include "RailEditor.h"
#include "Line3dDrawer.h"

RailEditor::~RailEditor() {

	controlPoints_.clear();

}


void RailEditor::Initialize() {

	for (int i = 0; i < 4; i++) {
		controlPoints_.push_back({ 0.0f,0.0f,0.0f });
	}

}


void RailEditor::Update() {


#ifdef _DEBUG
	ImGui::Begin("ControlPoint");
	for (int i = 0; i < controlPoints_.size(); i++) {
		std::string indexStr = std::to_string(i);
		if (ImGui::TreeNodeEx(("point" + indexStr).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {

			ImGui::DragFloat3("position", &controlPoints_[i].x, 0.01f);

			ImGui::TreePop();
		}
	}
	if (ImGui::Button("AddPoint")) {
		AddControlPoint(controlPoints_.back());
	}
	ImGui::End();
#endif // _DEBUG

}

void RailEditor::Draw() {

	std::vector<Vector3> pointsDrawing;
	const size_t segumentCount = 50;
	for (size_t i = 0; i < segumentCount + 1; i++) {
		float t = 1.0f / segumentCount * i;
		Vector3 pos = CatmullRom(controlPoints_, t);
		pointsDrawing.push_back(pos);
	}

	for (int i = 0; i < pointsDrawing.size() - 1;) {
		int index1 = i;
		i++;
		int index2 = i;
		Line3dDrawer::GetInstance()->DrawLine3d(pointsDrawing[index1], pointsDrawing[index2], Vector4{ 1.0f, 0.0f, 0.0f, 1.0f });
	}

}


void RailEditor::AddControlPoint([[maybe_unused]]const Vector3& point) {

	controlPoints_.push_back(point);

}
