#include "RailEditor.h"
#include "Line3dDrawer.h"
#include "CameraManager.h"
#include <numbers>

RailEditor::~RailEditor() {

	controlPoints_.clear();

}


void RailEditor::Initialize() {

	trans.scale = { 1.0f,1.0f,1.0f };

	controlPoints_.push_back({ 5.0f,2.0f,5.0f });
	controlPoints_.push_back({ 5.0f,2.0f,3.0f });
	controlPoints_.push_back({ 5.0f,2.0f,2.0f });
	controlPoints_.push_back({ 4.0f,2.0f,1.0f });
	controlPoints_.push_back({ 2.5f,2.0f,-1.0f });
	controlPoints_.push_back({ 0.5f,2.0f,-2.0f });
	controlPoints_.push_back({ -0.5f,2.0f,-2.0f });
	controlPoints_.push_back({ -2.5f,2.0f,-1.0f });
	controlPoints_.push_back({ -4.0f,2.0f,1.0f });
	controlPoints_.push_back({ -5.0f,2.0f,2.0f });
	controlPoints_.push_back({ -5.0f,2.0f,3.0f });
	controlPoints_.push_back({ -5.0f,2.0f,5.0f });




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
	ImGui::Checkbox("CameraSet", &isCamera);
	ImGui::End();
#endif // _DEBUG

	if (isCamera) {
		Camera* camera = CameraManager::GetInstance()->GetCamera();


		const float timelimitt = 2500.0f;
		if (time_ < timelimitt) {
			time_++;
		}
		else {
			time_ = 0;
			previousUp = { 0.0f,1.0f,0.0f };
		}

		float t = 1.0f / timelimitt * time_;
		Vector3 eye = CatmullRom(controlPoints_, t);

		float t2 = 1.0f / timelimitt * (time_ + 20);
		Vector3 target = CatmullRom(controlPoints_, t2);

		// カメラの位置を設定
		trans.translate = eye;

		// forwardベクトルを正規化
		Vector3 forward = target - eye;
		forward = forward.Normalize();

		// 1フレーム前のupベクトルを使って新しいrightベクトルを計算
		Vector3 right = Cross(previousUp, forward);
		right = right.Normalize();

		//// Z軸の回転を計算
		//trans.rotate.z = std::atan2(right.y, right.x);

		// 新しいupベクトルを計算 (forwardとrightベクトルの外積)
		Vector3 newUp = Cross(forward, right);
		newUp = newUp.Normalize();  // 正規化しておく

		// Y軸の回転を計算 (水平方向)
		trans.rotate.y = std::atan2(forward.x, forward.z);

		// X軸の回転を計算 (垂直方向)
		trans.rotate.x = std::atan2(-forward.y, std::sqrt(forward.x * forward.x + forward.z * forward.z));


		// 次フレーム用に現在のupベクトルを保存
		previousUp = newUp;


		camera->transform = trans;
	}

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
