#include "PlacedSpline.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <json.hpp>
#include "Engine/Graphics/Line/Line3dDrawer.h"
#include "Engine/Math/Matrix/MatrixCalculation.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#ifdef _DEBUGMODE
#include "Engine/Editor/Widget/GizmoHelper.h"
#include "ImGuizmo.h"
#endif // _DEBUGMODE

// 名前空間とクラス名が同名なので using namespace は書けない
namespace GameObject {

	using namespace Math;


	namespace {
		Vector3 ReadVector3(const nlohmann::json& data, const char* key, const Vector3& fallback) {
			if (data.contains(key) && data[key].is_array() && data[key].size() == 3) {
				const auto& v = data[key];
				return { v[0].get<float>(), v[1].get<float>(), v[2].get<float>() };
			}
			return fallback;
		}

		nlohmann::json ToArray(const Vector3& v) {
			return { v.x, v.y, v.z };
		}

		bool IsSamePoints(const std::vector<Spline::Point>& a, const std::vector<Spline::Point>& b) {
			if (a.size() != b.size()) {
				return false;
			}
			for (size_t i = 0; i < a.size(); ++i) {
				if (a[i].type != b[i].type || a[i].position != b[i].position) {
					return false;
				}
			}
			return true;
		}
	}

	void PlacedSpline::Create(const nlohmann::json& data) {
		SetName(data.value("name", std::string("Spline")));
		transform_.translate = ReadVector3(data, "position", { 0.0f,0.0f,0.0f });
		transform_.rotate = ReadVector3(data, "rotation", { 0.0f,0.0f,0.0f });
		transform_.scale = ReadVector3(data, "scale", { 1.0f,1.0f,1.0f });
		isClosed_ = data.value("closed", false);
		points_.clear();
		if (data.contains("points") && data["points"].is_array()) {
			for (const auto& pointData : data["points"]) {
				Spline::Point point;
				point.position = ReadVector3(pointData, "p", {});
				point.type = pointData.value("linear", false) ? Spline::PointType::Linear : Spline::PointType::Curve;
				points_.push_back(point);
			}
		}
		if (points_.size() < 2) {
			points_ = { { { 0.0f,0.0f,0.0f } }, { { 0.0f,0.0f,10.0f } } };
		}
		hasBuilt_ = false;
	}

	nlohmann::json PlacedSpline::ToJson() const {
		nlohmann::json data;
		data["name"] = GetName();
		data["position"] = ToArray(transform_.translate);
		data["rotation"] = ToArray(transform_.rotate);
		data["scale"] = ToArray(transform_.scale);
		data["closed"] = isClosed_;
		nlohmann::json points = nlohmann::json::array();
		for (const Spline::Point& point : points_) {
			nlohmann::json pointData;
			pointData["p"] = ToArray(point.position);
			if (point.type == Spline::PointType::Linear) {
				pointData["linear"] = true;
			}
			points.push_back(std::move(pointData));
		}
		data["points"] = std::move(points);
		return data;
	}

	bool PlacedSpline::IsDirty() const {
		if (!hasBuilt_ || builtClosed_ != isClosed_ || !IsSamePoints(builtPoints_, points_)) {
			return true;
		}
		const Matrix4x4 world = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
		return std::memcmp(&world, &builtMatrix_, sizeof(Matrix4x4)) != 0;
	}

	const Spline& PlacedSpline::GetWorldSpline() const {
		if (IsDirty()) {
			builtMatrix_ = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
			builtPoints_ = points_;
			builtClosed_ = isClosed_;
			std::vector<Spline::Point> worldPoints = points_;
			for (Spline::Point& point : worldPoints) {
				point.position = Transform(point.position, builtMatrix_);
			}
			worldSpline_.Set(worldPoints, isClosed_);
			hasBuilt_ = true;
		}
		return worldSpline_;
	}

	void PlacedSpline::Draw([[maybe_unused]] bool is) {
	#ifdef _DEBUGMODE
		const Spline& spline = GetWorldSpline();
		Graphics::Line3dDrawer* line = Graphics::Line3dDrawer::GetInstance();
		const Vector4 curveColor = { 1.0f,0.85f,0.2f,1.0f };
		constexpr int kStepsPerSegment = 12;
		const int steps = spline.GetSegmentCount() * kStepsPerSegment;
		Vector3 previous = spline.Evaluate(0.0f);
		for (int i = 1; i <= steps; ++i) {
			const Vector3 position = spline.Evaluate(static_cast<float>(i) / kStepsPerSegment);
			line->DrawLine3d(previous, position, curveColor);
			previous = position;
		}
		// 点の十字。選んでいる点は大きく白く
		const auto& points = spline.GetPoints();
		for (size_t i = 0; i < points.size(); ++i) {
			const bool isSelected = static_cast<int>(i) == selectedPoint_;
			const float size = isSelected ? 0.6f : 0.3f;
			const Vector4 color = isSelected ? Vector4{ 1.0f,1.0f,1.0f,1.0f } : Vector4{ 1.0f,0.5f,0.1f,1.0f };
			const Vector3& p = points[i].position;
			line->DrawLine3d({ p.x - size, p.y, p.z }, { p.x + size, p.y, p.z }, color);
			line->DrawLine3d({ p.x, p.y - size, p.z }, { p.x, p.y + size, p.z }, color);
			line->DrawLine3d({ p.x, p.y, p.z - size }, { p.x, p.y, p.z + size }, color);
		}
	#endif // _DEBUGMODE
	}

	void PlacedSpline::ParameterGUI() {
	#ifdef _DEBUGMODE
		char name[64] = {};
		GetName().copy(name, sizeof(name) - 1);
		if (ImGui::InputText("Name", name, sizeof(name))) {
			SetName(name);
		}
		ImGui::Checkbox("Closed", &isClosed_);
		ImGui::Text("Length %.2f m / %zu points", GetWorldSpline().GetLength(), points_.size());

		ImGui::SeparatorText("Points");
		for (int i = 0; i < static_cast<int>(points_.size()); ++i) {
			ImGui::PushID(i);
			const char* type = (points_[i].type == Spline::PointType::Linear) ? "Linear" : "Curve";
			const std::string label = std::to_string(i) + "  " + type;
			if (ImGui::Selectable(label.c_str(), selectedPoint_ == i)) {
				selectedPoint_ = (selectedPoint_ == i) ? -1 : i;
			}
			ImGui::PopID();
		}
		if (selectedPoint_ >= static_cast<int>(points_.size())) {
			selectedPoint_ = -1;
		}

		// 足すのは選んだ点の後ろ (無ければ最後)。前の区間の向きへ伸ばす
		if (ImGui::Button("Add point")) {
			const int at = (selectedPoint_ >= 0) ? selectedPoint_ : static_cast<int>(points_.size()) - 1;
			const Vector3 base = points_[at].position;
			Vector3 step = { 0.0f,0.0f,5.0f };
			if (at + 1 < static_cast<int>(points_.size())) {
				step = (points_[at + 1].position - base) * 0.5f;
			} else if (at > 0) {
				step = base - points_[at - 1].position;
			}
			points_.insert(points_.begin() + at + 1, Spline::Point{ base + step, points_[at].type });
			selectedPoint_ = at + 1;
		}
		ImGui::SameLine();
		ImGui::BeginDisabled(selectedPoint_ < 0 || points_.size() <= 2);
		if (ImGui::Button("Remove point")) {
			points_.erase(points_.begin() + selectedPoint_);
			selectedPoint_ = (std::min)(selectedPoint_, static_cast<int>(points_.size()) - 1);
		}
		ImGui::EndDisabled();

		if (selectedPoint_ >= 0) {
			Spline::Point& point = points_[selectedPoint_];
			ImGui::DragFloat3("Local position", &point.position.x, 0.05f);
			int type = static_cast<int>(point.type);
			if (ImGui::Combo("Type", &type, "Curve\0Linear\0")) {
				point.type = static_cast<Spline::PointType>(type);
			}
			ImGui::Checkbox("Point gizmo", &isPointGizmo_);
			if (isPointGizmo_) {
				// 選んだ点だけをワールドで動かす。物のギズモと別の ID にする
				const Matrix4x4 world = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
				Trans pointTrans;
				pointTrans.translate = Transform(point.position, world);
				pointTrans.scale = { 1.0f,1.0f,1.0f };
				const Vector3 before = pointTrans.translate;
				ImGuizmo::PushID(this);
				Editor::GizmoHelper::ManipulateSimple(pointTrans, Editor::GizmoOperation::Translate);
				ImGuizmo::PopID();
				// 動かしたときだけ戻す。毎フレーム往復させると誤差で少しずつずれる
				if (pointTrans.translate != before) {
					point.position = Transform(pointTrans.translate, Inverse(world));
				}
			}
		}
	#endif // _DEBUGMODE
	}

}
