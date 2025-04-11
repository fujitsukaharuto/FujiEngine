#include "ParticleEmitter.h"
#include "ParticleManager.h"
#include "Math/Random/Random.h"
#include "ImGuiManager.h"
#include "Model/Line3dDrawer.h"
#include <iostream>
#include <fstream>

ParticleEmitter::ParticleEmitter() {
}

ParticleEmitter::~ParticleEmitter() {
}

#ifdef _DEBUG
void ParticleEmitter::DebugGUI() {
	if (ImGui::CollapsingHeader(name_.c_str())) {
		if (ImGui::TreeNode("emitter")) {
			ImGui::DragFloat3("pos", &pos_.x, 0.01f);
			int im_Count = int(count_);
			ImGui::DragInt("count", &im_Count, 1, 0, 10);
			count_ = uint32_t(im_Count);
			ImGui::DragFloat("frenquencyTime", &frequencyTime_, 0.1f, 1.0f, 600.0f);
			ImGui::DragFloat3("emitSizeMax", &emitSizeMax_.x, 0.01f);
			ImGui::DragFloat3("emitSizeMin", &emitSizeMin_.x, 0.01f);
			ImGui::TreePop();
		}
		ImGui::Separator();
		if (ImGui::TreeNode("particle")) {

			ImGui::ColorEdit4("colorMin", &para_.colorMin.x);
			ImGui::ColorEdit4("colorMax", &para_.colorMax.x);

			ImGui::DragFloat("lifetime", &grain_.lifeTime_, 0.1f);
			if (ImGui::TreeNode("typeSelect")) {
				ImGui::Combo("sizeType##type", &grain_.type_, "kNormal\0kShift\0kSin\0");
				ImGui::Combo("speedType##type", &grain_.speedType_, "kConstancy\0kChange\0kReturn\0kCenter\0");
				ImGui::Combo("rotateType##type", &grain_.rotateType_, "kUsually\0kVelocityR\0kRandomR\0");
				ImGui::Combo("colorType##type", &grain_.colorType_, "kDefault\0kRandom\0");
				ImGui::TreePop();
			}
			ImGui::SeparatorText("size");
			ImGui::DragFloat2("startSize", &grain_.startSize_.x, 0.01f);
			ImGui::DragFloat2("endSize", &grain_.endSize_.x, 0.01f);
			ImGui::SeparatorText("rotate");
			ImGui::DragFloat3("rotate", &particleRotate_.x, 0.01f);
			ImGui::SeparatorText("randSpeed");
			ImGui::DragFloat2("speedX", &para_.speedx.x, 0.01f);
			ImGui::DragFloat2("speedY", &para_.speedy.x, 0.01f);
			ImGui::DragFloat2("speedZ", &para_.speedz.x, 0.01f);
			if (grain_.speedType_ == static_cast<int>(SpeedType::kReturn) || grain_.speedType_ == static_cast<int>(SpeedType::kCenter)) {
				ImGui::DragFloat("returnPower", &grain_.returnPower_, 0.001f);
			}
			ImGui::SeparatorText("accele");
			ImGui::DragFloat3("accele", &grain_.accele_.x, 0.01f);
			ImGui::SeparatorText("billBoard");
			ImGui::Checkbox("BillBoard", &grain_.isBillBoard_);
			if (grain_.isBillBoard_) {
				int billPattern = static_cast<int>(grain_.pattern_);
				ImGui::RadioButton("XYZ", &billPattern, 0); ImGui::SameLine();
				ImGui::RadioButton("X", &billPattern, 1); ImGui::SameLine();
				ImGui::RadioButton("Y", &billPattern, 2); ImGui::SameLine();
				ImGui::RadioButton("Z", &billPattern, 3);
				grain_.pattern_ = static_cast<BillBoardPattern>(billPattern);
			}
			ImGui::Checkbox("SizeCheck", &isDrawSize_);
			ImGui::TreePop();
		}
		if (ImGui::Button("save")) {
			Save();
		}
	}
}
void ParticleEmitter::DrawSize() {
	if (isDrawSize_) {

		Matrix4x4 worldMatrix = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, pos_);
		if (parent_) {
			const Matrix4x4& parentWorldMatrix = parent_->GetWorldMat();
			worldMatrix = Multiply(worldMatrix, parentWorldMatrix);
		}

		Vector3 points[8];
		points[0] = emitSizeMin_;
		points[0] += {worldMatrix.m[3][0], worldMatrix.m[3][1], worldMatrix.m[3][2]};
		points[1] = { emitSizeMax_.x,emitSizeMin_.y,emitSizeMin_.z };
		points[1] += {worldMatrix.m[3][0], worldMatrix.m[3][1], worldMatrix.m[3][2]};
		points[2] = { emitSizeMax_.x,emitSizeMin_.y,emitSizeMax_.z };
		points[2] += {worldMatrix.m[3][0], worldMatrix.m[3][1], worldMatrix.m[3][2]};
		points[3] = { emitSizeMin_.x,emitSizeMin_.y,emitSizeMax_.z };
		points[3] += {worldMatrix.m[3][0], worldMatrix.m[3][1], worldMatrix.m[3][2]};

		points[4] = emitSizeMax_;
		points[4] += {worldMatrix.m[3][0], worldMatrix.m[3][1], worldMatrix.m[3][2]};
		points[5] = { emitSizeMin_.x,emitSizeMax_.y,emitSizeMax_.z };
		points[5] += {worldMatrix.m[3][0], worldMatrix.m[3][1], worldMatrix.m[3][2]};
		points[6] = { emitSizeMin_.x,emitSizeMax_.y,emitSizeMin_.z };
		points[6] += {worldMatrix.m[3][0], worldMatrix.m[3][1], worldMatrix.m[3][2]};
		points[7] = { emitSizeMax_.x,emitSizeMax_.y,emitSizeMin_.z };
		points[7] += {worldMatrix.m[3][0], worldMatrix.m[3][1], worldMatrix.m[3][2]};

		Line3dDrawer::GetInstance()->DrawLine3d(points[0], points[1], Vector4{ 1.0f, 0.0f, 0.0f, 1.0f });
		Line3dDrawer::GetInstance()->DrawLine3d(points[1], points[2], Vector4{ 1.0f, 0.0f, 0.0f, 1.0f });
		Line3dDrawer::GetInstance()->DrawLine3d(points[2], points[3], Vector4{ 1.0f, 0.0f, 0.0f, 1.0f });
		Line3dDrawer::GetInstance()->DrawLine3d(points[3], points[0], Vector4{ 1.0f, 0.0f, 0.0f, 1.0f });

		Line3dDrawer::GetInstance()->DrawLine3d(points[4], points[5], Vector4{ 1.0f, 0.0f, 0.0f, 1.0f });
		Line3dDrawer::GetInstance()->DrawLine3d(points[5], points[6], Vector4{ 1.0f, 0.0f, 0.0f, 1.0f });
		Line3dDrawer::GetInstance()->DrawLine3d(points[6], points[7], Vector4{ 1.0f, 0.0f, 0.0f, 1.0f });
		Line3dDrawer::GetInstance()->DrawLine3d(points[7], points[4], Vector4{ 1.0f, 0.0f, 0.0f, 1.0f });

		Line3dDrawer::GetInstance()->DrawLine3d(points[0], points[6], Vector4{ 1.0f, 0.0f, 0.0f, 1.0f });
		Line3dDrawer::GetInstance()->DrawLine3d(points[1], points[7], Vector4{ 1.0f, 0.0f, 0.0f, 1.0f });
		Line3dDrawer::GetInstance()->DrawLine3d(points[2], points[4], Vector4{ 1.0f, 0.0f, 0.0f, 1.0f });
		Line3dDrawer::GetInstance()->DrawLine3d(points[3], points[5], Vector4{ 1.0f, 0.0f, 0.0f, 1.0f });

	}
}
#endif // _DEBUG

void ParticleEmitter::Emit() {
	Matrix4x4 worldMatrix = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, pos_);
	if (parent_) {
		const Matrix4x4& parentWorldMatrix = parent_->GetWorldMat();
		worldMatrix = Multiply(worldMatrix, parentWorldMatrix);
	}

	if (time_ <= 0) {
		for (uint32_t i = 0; i < count_; i++) {
			Vector3 posAddSize{};
			posAddSize = Random::GetVector3({ emitSizeMin_.x,emitSizeMax_.x }, { emitSizeMin_.y,emitSizeMax_.y }, { emitSizeMin_.z,emitSizeMax_.z });
			posAddSize += {worldMatrix.m[3][0], worldMatrix.m[3][1], worldMatrix.m[3][2]};

			if (grain_.speedType_ == static_cast<int>(SpeedType::kCenter)) {
				grain_.speed_ = (pos_ - posAddSize) * grain_.returnPower_;
			}

			ParticleManager::Emit(name_, posAddSize, particleRotate_, grain_, para_, 1);
		}
		time_ = frequencyTime_;
	} else {
		time_--;
	}
}

void ParticleEmitter::Burst() {
	Matrix4x4 worldMatrix = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, pos_);
	if (parent_) {
		const Matrix4x4& parentWorldMatrix = parent_->GetWorldMat();
		worldMatrix = Multiply(worldMatrix, parentWorldMatrix);
	}

	for (uint32_t i = 0; i < count_; i++) {
		Vector3 posAddSize{};
		posAddSize = Random::GetVector3({ emitSizeMin_.x,emitSizeMax_.x }, { emitSizeMin_.y,emitSizeMax_.y }, { emitSizeMin_.z,emitSizeMax_.z });
		posAddSize += {worldMatrix.m[3][0], worldMatrix.m[3][1], worldMatrix.m[3][2]};

		if (grain_.speedType_ == static_cast<int>(SpeedType::kCenter)) {
			grain_.speed_ = (pos_ - posAddSize).Normalize() * grain_.returnPower_;
		}

		ParticleManager::Emit(name_, posAddSize, particleRotate_, grain_, para_, 1);
	}
}

void ParticleEmitter::BurstAnime() {
	ParticleManager::EmitAnime(name_, pos_, animeData_, para_, count_);
}

void ParticleEmitter::RandomSpeed(const Vector2& x, const Vector2& y, const Vector2& z) {
	para_.speedx = x;
	para_.speedy = y;
	para_.speedz = z;
}

void ParticleEmitter::RandomTranslate(const Vector2& x, const Vector2& y, const Vector2& z) {
	para_.transx = x;
	para_.transy = y;
	para_.transz = z;
}

void ParticleEmitter::Save() {
	json j;

	j.push_back(json::array({ pos_.x,pos_.y,pos_.z }));
	j.push_back(json::array({ particleRotate_.x,particleRotate_.y,particleRotate_.z }));
	j.push_back(json::array({ emitSizeMax_.x,emitSizeMax_.y,emitSizeMax_.z }));
	j.push_back(json::array({ emitSizeMin_.x,emitSizeMin_.y,emitSizeMin_.z }));

	j.push_back(count_);
	j.push_back(frequencyTime_);

	j.push_back(grain_.lifeTime_);
	j.push_back(json::array({ grain_.accele_.x,grain_.accele_.y,grain_.accele_.z }));
	j.push_back(json::array({ grain_.speed_.x,grain_.speed_.y,grain_.speed_.z }));

	j.push_back(grain_.type_);
	j.push_back(grain_.speedType_);
	j.push_back(grain_.rotateType_);
	j.push_back(grain_.colorType_);

	j.push_back(grain_.returnPower_);

	j.push_back(json::array({ grain_.startSize_.x,grain_.startSize_.y }));
	j.push_back(json::array({ grain_.endSize_.x,grain_.endSize_.y }));

	j.push_back(grain_.isBillBoard_);

	j.push_back(static_cast<int>(grain_.pattern_));

	j.push_back(json::array({ para_.speedx.x,para_.speedx.y }));
	j.push_back(json::array({ para_.speedy.x,para_.speedy.y }));
	j.push_back(json::array({ para_.speedz.x,para_.speedz.y }));

	j.push_back(json::array({ para_.transx.x,para_.transx.y }));
	j.push_back(json::array({ para_.transy.x,para_.transy.y }));
	j.push_back(json::array({ para_.transz.x,para_.transz.y }));

	j.push_back(json::array({ para_.colorMin.x,para_.colorMin.y,para_.colorMin.z,para_.colorMin.w }));
	j.push_back(json::array({ para_.colorMax.x,para_.colorMax.y,para_.colorMax.z,para_.colorMax.w }));


	std::ofstream file(kDirectoryPath_ + name_ + ".json");
	if (file.is_open()) {
		file << j.dump(4);
		file.close();
	}
}

void ParticleEmitter::Load(const std::string& filename) {
	std::ifstream file(kDirectoryPath_ + filename + ".json");
	if (!file.is_open()) {
		return;
	}

	json j;
	file >> j;
	file.close();

	// データをロードしてメンバーに復元
	int index = 0;
	pos_ = Vector3(j[index][0], j[index][1], j[index][2]);
	index++;
	particleRotate_ = Vector3(j[index][0], j[index][1], j[index][2]);
	index++;

	emitSizeMax_ = Vector3(j[index][0], j[index][1], j[index][2]);
	index++;
	emitSizeMin_ = Vector3(j[index][0], j[index][1], j[index][2]);
	index++;

	count_ = j[index].get<int>();
	index++;
	frequencyTime_ = j[index].get<float>();
	index++;
	grain_.lifeTime_ = j[index].get<float>();
	index++;

	grain_.accele_ = Vector3(j[index][0], j[index][1], j[index][2]);
	index++;
	grain_.speed_ = Vector3(j[index][0], j[index][1], j[index][2]);
	index++;

	grain_.type_ = j[index].get<int>();
	index++;
	grain_.speedType_ = j[index].get<int>();
	index++;
	grain_.rotateType_ = j[index].get<int>();
	index++;
	grain_.colorType_ = j[index].get<int>();
	index++;

	grain_.returnPower_ = j[index].get<float>();
	index++;

	grain_.startSize_ = Vector2(j[index][0], j[index][1]);
	index++;
	grain_.endSize_ = Vector2(j[index][0], j[index][1]);
	index++;

	grain_.isBillBoard_ = j[index].get<bool>();
	index++;

	grain_.pattern_ = static_cast<BillBoardPattern>(j[index].get<int>());
	index++;

	para_.speedx = Vector2(j[index][0], j[index][1]);
	index++;
	para_.speedy = Vector2(j[index][0], j[index][1]);
	index++;
	para_.speedz = Vector2(j[index][0], j[index][1]);
	index++;

	para_.transx = Vector2(j[index][0], j[index][1]);
	index++;
	para_.transy = Vector2(j[index][0], j[index][1]);
	index++;
	para_.transz = Vector2(j[index][0], j[index][1]);
	index++;

	para_.colorMin = Vector4(j[index][0], j[index][1], j[index][2], j[index][3]);
	index++;
	para_.colorMax = Vector4(j[index][0], j[index][1], j[index][2], j[index][3]);
	index++;
}

Vector3 ParticleEmitter::GetWorldPos() {
	Matrix4x4 worldMatrix = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, pos_);
	if (parent_) {
		const Matrix4x4& parentWorldMatrix = parent_->GetWorldMat();
		worldMatrix = Multiply(worldMatrix, parentWorldMatrix);
	}
	return Vector3{ worldMatrix.m[3][0], worldMatrix.m[3][1], worldMatrix.m[3][2] };
}