#include "RailEditor.h"
#include "Line/Line3dDrawer.h"
#include "CameraManager.h"
#include <numbers>
#include <variant>
#include "Scene/SceneManager.h"

RailEditor::~RailEditor() {

	controlPoints_.clear();

	for (auto& i : rails) {
		delete i;
	}
}


void RailEditor::Initialize() {

	trans.scale = { 1.0f,1.0f,1.0f };


	for (int i = 0; i < (reilCount_); i++) {
		Object3d* newModel = new Object3d();
		newModel->Create("rail2.obj");
		rails.push_back(newModel);
	}

	Load("rails.json");

	track_.reset(new Object3d);
	track_->Create("track.obj");
	track_->transform.scale.z = 3.5f;

	SetRail();

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
	if (ImGui::Button("DeletePoint")) {
		controlPoints_.pop_back();
	}
	if (ImGui::Button("SetModel")) {
		SetRail();
	}
	if (ImGui::Button("UnsetModel")) {
		for (int i = 0; i < (reilCount_ / 3.0f); i++) {
			
			rails[i]->transform.rotate = { 0.0f,0.0f,0.0f };
			rails[i]->transform.translate = { 0.0f,0.0f,0.0f };
		}
	}
	if (ImGui::Button("SAVE")) {
		Save("rails.json");
	}
	ImGui::End();

	ImGui::Begin("railcamera");
	ImGui::Checkbox("CameraSet", &isCamera);
	if (ImGui::Button("resetCamera")) {
		time_ = 0.0f;
	}

	ImGui::End();

#endif // _DEBUG

	if (isCamera) {
		Camera* camera = CameraManager::GetInstance()->GetCamera();


		const float timelimitt = 9000.0f;
		if (time_ < timelimitt) {
			time_++;
		}
		else {
			time_ = 0;
			previousUp = { 0.0f,1.0f,0.0f };
		}
		if (time_ >= 8900.0f) {
			SceneManager::GetInstance()->ChangeScene("RESULT", 30.0f);
		}

		Vector3 offset = { 0.0f,0.75f,0.0f };
		float t = 1.0f / timelimitt * time_;
		Vector3 eye = CatmullRom(controlPoints_, t);
		eye += offset;

		float t2 = 1.0f / timelimitt * (time_ + 60);
		Vector3 target = CatmullRom(controlPoints_, t2);
		target += offset;

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



		offset = { 0.0f,0.05f,0.0f };
		t = 1.0f / timelimitt * (time_+10.0f);
		eye = CatmullRom(controlPoints_, t);
		eye += offset;

		t2 = 1.0f / timelimitt * (time_ + 40.0f);
		target = CatmullRom(controlPoints_, t2);
		target += offset;

		// カメラの位置を設定
		track_->transform.translate = eye;

		// forwardベクトルを正規化
		forward = target - eye;
		forward = forward.Normalize();

		// 1フレーム前のupベクトルを使って新しいrightベクトルを計算
		right = Cross(previousUp, forward);
		right = right.Normalize();

		//// Z軸の回転を計算
		//trans.rotate.z = std::atan2(right.y, right.x);

		// 新しいupベクトルを計算 (forwardとrightベクトルの外積)
		newUp = Cross(forward, right);
		newUp = newUp.Normalize();  // 正規化しておく

		// Y軸の回転を計算 (水平方向)
		track_->transform.rotate.y = std::atan2(forward.x, forward.z);

		// X軸の回転を計算 (垂直方向)
		track_->transform.rotate.x = std::atan2(-forward.y, std::sqrt(forward.x * forward.x + forward.z * forward.z));




	}

}

void RailEditor::RailDarw() {
	for (auto& i : rails) {
		if (IsVisibleFromCamera(i->transform.translate, 2.0f, CameraManager::GetInstance()->GetCamera())) {
			if (i->transform.translate != Vector3{0.0f, 0.0f, 0.0f}) {
				i->Draw();
			}
		}
	}
	track_->Draw();
}

void RailEditor::Draw() {

#ifdef _DEBUG

	std::vector<Vector3> pointsDrawing;
	const size_t segumentCount = reilCount_;
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

#endif // _DEBUG
	
}


void RailEditor::AddControlPoint([[maybe_unused]]const Vector3& point) {

	controlPoints_.push_back(point);

}

void RailEditor::SetRail() {


	std::vector<Vector3> pointsDrawing;
	const size_t segumentCount = reilCount_;
	for (size_t i = 0; i < segumentCount + 1; i++) {
		float t = 1.0f / segumentCount * i;
		Vector3 pos = CatmullRom(controlPoints_, t);
		pointsDrawing.push_back(pos);
	}

	for (int i = 0; i < pointsDrawing.size() - 2; i++) {
		Vector3 norTarget = (pointsDrawing[i + 1] - pointsDrawing[i]).Normalize();

		/*Vector3 currentDir = { 0.0f,0.0f,1.0f };
		Vector3 axis = Cross(currentDir, norTarget);
		float angle = acos(currentDir * norTarget);
		rails[i]->transform.rotate = { axis.x * angle, axis.y * angle, 0.0f };*/


		float pitch = asinf(norTarget.y);
		float yaw = atan2f(norTarget.x, norTarget.z);
		float roll = 0.0f;

		if (i != 0) {
			float length = (rails[i - 1]->transform.translate - pointsDrawing[i]).Length();
			if (length < 0.5f) {
				continue;
			}
		}

		rails[i]->transform.rotate = { -pitch,yaw,roll };

		rails[i]->transform.translate = pointsDrawing[i];
		

	}
}

void RailEditor::Save(const std::string& fileName) {

	json j;
	for (Vector3 point : controlPoints_) {
		j.push_back(json::array({ point.x,point.y,point.z }));
	}

	std::ofstream file(kDirectoryPath + fileName);
	if (file.is_open()) {
		file << j.dump(4); // インデント4で出力
		file.close();
	}

}

void RailEditor::Load(const std::string& fileName) {

	std::ifstream file(kDirectoryPath + fileName);
	if (file.is_open()) {
		json j;
		file >> j;

		controlPoints_.clear(); // 既存のデータをクリア
		for (const auto& item : j) {
			Vector3 value = { item.at(0),item.at(1),item.at(2) };
			controlPoints_.push_back(value);
		}

		file.close();
	}

}

bool RailEditor::IsVisibleFromCamera(const Vector3& sphereCenter, float sphereRadius, Camera* camera) {

	for (const auto& plane : { camera->frustum.nearPlane, camera->frustum.farPlane, camera->frustum.rightPlane, camera->frustum.leftPlane, camera->frustum.topPlane, camera->frustum.bottomPlane }) {
		// 球の中心から平面への距離を計算
		float distance = DistanceToPoint(plane, sphereCenter);

		// 球が完全に平面の外側にあるかどうかチェック
		if (distance < -sphereRadius) {
			return false; // 球が平面の外側にあるため描画不要
		}
	}
	return true; // 全ての平面でチェックを通過したので描画対象
}

float RailEditor::DistanceToPoint(const Plane& plane, const Vector3& point) {
	return plane.normal * point + plane.distance;
}
