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

void ParticleEmitter::DebugGUI() {
#ifdef _DEBUG
	if (ImGui::CollapsingHeader(name_.c_str())) {
		ImGui::Indent();
		if (ImGui::CollapsingHeader("Emitter")) {
			ImGui::Indent();

			ImGui::DragFloat3("Pos", &pos_.x, 0.01f);
			int im_Count = int(count_);
			ImGui::DragInt("Count", &im_Count, 1, 0, 10);
			count_ = uint32_t(im_Count);
			ImGui::DragFloat("FrenquencyTime", &frequencyTime_, 0.1f, 1.0f, 600.0f);
			ImGui::DragFloat("LifeTime", &grain_.lifeTime_, 0.1f);
			ImGui::SeparatorText("EmitterSize");
			ImGui::DragFloat3("EmitSizeMax", &emitSizeMax_.x, 0.01f);
			ImGui::DragFloat3("EmitSizeMin", &emitSizeMin_.x, 0.01f);

			ImGui::Unindent();
		}

		if (ImGui::CollapsingHeader("Particle")) {
			ImGui::Indent();
			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Selected;
			if (ImGui::TreeNodeEx("Color", flags)) {
				ImGui::Combo("ColorType##type", &grain_.colorType_, "kDefault\0kRandom\0");
				ImGui::Separator();
				if (grain_.colorType_ != static_cast<int>(ColorType::kRandom)) {
					ImGui::ColorEdit4("Color", &para_.colorMax.x);
				} else {
					ImGui::ColorEdit4("ColorMax", &para_.colorMax.x);
					ImGui::ColorEdit4("ColorMin", &para_.colorMin.x);
				}
				ImGui::Checkbox("IsColorFade", &grain_.isColorFade_);
				ImGui::Checkbox("IsAutoUV", &grain_.isAutoUVMove_);
				if (grain_.isAutoUVMove_) {
					ImGui::DragFloat2("AutoUVSpeed", &grain_.autoUVSpeed_.x, 0.01f);
				}
				ImGui::Checkbox("IsColorFadeIn", &grain_.isColorFadeIn_);
				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Size", flags)) {
				ImGui::Combo("SizeType##type", &grain_.type_, "kNormal\0kShift\0kSin\0");
				ImGui::Separator();
				if (grain_.type_ == static_cast<int>(SizeType::kNormal)) {
					ImGui::DragFloat2("Size", &grain_.startSize_.x, 0.01f);
				} else {
					ImGui::DragFloat2("StartSize", &grain_.startSize_.x, 0.01f);
					ImGui::DragFloat2("EndSize", &grain_.endSize_.x, 0.01f);
				}
				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Rotate", flags)) {
				ImGui::Combo("RotateType##type", &grain_.rotateType_, "kUsually\0kVelocityR\0kRandomR\0");
				ImGui::Separator();
				ImGui::DragFloat3("Rotate", &particleRotate_.x, 0.01f);
				if (grain_.rotateType_ == static_cast<int>(RotateType::kRandomR)) {
					ImGui::Checkbox("ContinuouslyRotate", &grain_.isContinuouslyRotate_);
				}
				ImGui::SeparatorText("BillBoard");
				ImGui::Checkbox("BillBoard", &grain_.isBillBoard_);
				if (grain_.isBillBoard_) {
					int billPattern = static_cast<int>(grain_.pattern_);
					ImGui::RadioButton("XYZ", &billPattern, 0); ImGui::SameLine();
					ImGui::RadioButton("X", &billPattern, 1); ImGui::SameLine();
					ImGui::RadioButton("Y", &billPattern, 2); ImGui::SameLine();
					ImGui::RadioButton("Z", &billPattern, 3); ImGui::SameLine();
					ImGui::RadioButton("XY", &billPattern, 4);
					grain_.pattern_ = static_cast<BillBoardPattern>(billPattern);
				}
				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Speed", flags)) {
				ImGui::Combo("SpeedType##type", &grain_.speedType_, "kConstancy\0kChange\0kReturn\0kCenter\0");
				ImGui::Separator();

				ImGui::DragFloat2("SpeedX", &para_.speedx.x, 0.01f);
				ImGui::DragFloat2("SpeedY", &para_.speedy.x, 0.01f);
				ImGui::DragFloat2("SpeedZ", &para_.speedz.x, 0.01f);
				if (grain_.speedType_ == static_cast<int>(SpeedType::kReturn) || grain_.speedType_ == static_cast<int>(SpeedType::kCenter)) {
					ImGui::DragFloat("ReturnPower", &grain_.returnPower_, 0.001f);
				}
				if (grain_.speedType_ == static_cast<int>(SpeedType::kReturn)) {
					ImGui::SeparatorText("Accele");
					ImGui::DragFloat3("Accele", &grain_.accele_.x, 0.01f);
				}
				ImGui::TreePop();
			}

			/*if (ImGui::TreeNodeEx("Color", flags)) {

				ImGui::TreePop();
			}*/

			ImGui::Unindent();
		}

		ImGui::Checkbox("SizeCheck", &isDrawSize_);
		ImGui::Checkbox("IsEmitte", &isEmit_);
		ImGui::Checkbox("IsDistance", &isDistanceComplement_);

		if (ImGui::Button("save")) {
			Save();
		}
		ImGui::Unindent();
	}
#endif // _DEBUG
}

void ParticleEmitter::DrawSize() {
#ifdef _DEBUG
	if (isDrawSize_) {


		worldMatrix_ = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, pos_);
		if (parent_) {
			const Matrix4x4& parentWorldMatrix = parent_->GetWorldMat();
			// スケール成分を除去した親ワールド行列を作成
			Matrix4x4 noScaleParentMatrix = parentWorldMatrix;

			// 各軸ベクトルの長さ（スケール）を計算
			Vector3 xAxis = { parentWorldMatrix.m[0][0], parentWorldMatrix.m[1][0], parentWorldMatrix.m[2][0] };
			Vector3 yAxis = { parentWorldMatrix.m[0][1], parentWorldMatrix.m[1][1], parentWorldMatrix.m[2][1] };
			Vector3 zAxis = { parentWorldMatrix.m[0][2], parentWorldMatrix.m[1][2], parentWorldMatrix.m[2][2] };

			float xLen = Vector3::Length(xAxis);
			float yLen = Vector3::Length(yAxis);
			float zLen = Vector3::Length(zAxis);

			// 正規化（スケールを除去）
			for (int i = 0; i < 3; ++i) {
				noScaleParentMatrix.m[i][0] /= xLen;
				noScaleParentMatrix.m[i][1] /= yLen;
				noScaleParentMatrix.m[i][2] /= zLen;
			}

			// 変換はそのまま（位置は影響受けてOKなら）
			worldMatrix_ = Multiply(worldMatrix_, noScaleParentMatrix);
		}


		Vector3 points[8];
		points[0] = emitSizeMin_;
		points[0] += {worldMatrix_.m[3][0], worldMatrix_.m[3][1], worldMatrix_.m[3][2]};
		points[1] = { emitSizeMax_.x,emitSizeMin_.y,emitSizeMin_.z };
		points[1] += {worldMatrix_.m[3][0], worldMatrix_.m[3][1], worldMatrix_.m[3][2]};
		points[2] = { emitSizeMax_.x,emitSizeMin_.y,emitSizeMax_.z };
		points[2] += {worldMatrix_.m[3][0], worldMatrix_.m[3][1], worldMatrix_.m[3][2]};
		points[3] = { emitSizeMin_.x,emitSizeMin_.y,emitSizeMax_.z };
		points[3] += {worldMatrix_.m[3][0], worldMatrix_.m[3][1], worldMatrix_.m[3][2]};

		points[4] = emitSizeMax_;
		points[4] += {worldMatrix_.m[3][0], worldMatrix_.m[3][1], worldMatrix_.m[3][2]};
		points[5] = { emitSizeMin_.x,emitSizeMax_.y,emitSizeMax_.z };
		points[5] += {worldMatrix_.m[3][0], worldMatrix_.m[3][1], worldMatrix_.m[3][2]};
		points[6] = { emitSizeMin_.x,emitSizeMax_.y,emitSizeMin_.z };
		points[6] += {worldMatrix_.m[3][0], worldMatrix_.m[3][1], worldMatrix_.m[3][2]};
		points[7] = { emitSizeMax_.x,emitSizeMax_.y,emitSizeMin_.z };
		points[7] += {worldMatrix_.m[3][0], worldMatrix_.m[3][1], worldMatrix_.m[3][2]};

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
#endif // _DEBUG
}

void ParticleEmitter::Emit() {
	if (time_ <= 0) {
		if (isDistanceComplement_) {
			previousWorldPos_ = currentWorldPos_;
		}

		worldMatrix_ = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, pos_);
		if (parent_) {
			const Matrix4x4& parentWorldMatrix = parent_->GetWorldMat();
			// スケール成分を除去した親ワールド行列を作成
			Matrix4x4 noScaleParentMatrix = parentWorldMatrix;

			// 各軸ベクトルの長さ（スケール）を計算
			Vector3 xAxis = { parentWorldMatrix.m[0][0], parentWorldMatrix.m[1][0], parentWorldMatrix.m[2][0] };
			Vector3 yAxis = { parentWorldMatrix.m[0][1], parentWorldMatrix.m[1][1], parentWorldMatrix.m[2][1] };
			Vector3 zAxis = { parentWorldMatrix.m[0][2], parentWorldMatrix.m[1][2], parentWorldMatrix.m[2][2] };

			float xLen = Vector3::Length(xAxis);
			float yLen = Vector3::Length(yAxis);
			float zLen = Vector3::Length(zAxis);

			// 正規化（スケールを除去）
			for (int i = 0; i < 3; ++i) {
				noScaleParentMatrix.m[i][0] /= xLen;
				noScaleParentMatrix.m[i][1] /= yLen;
				noScaleParentMatrix.m[i][2] /= zLen;
			}

			// 変換はそのまま（位置は影響受けてOKなら）
			worldMatrix_ = Multiply(worldMatrix_, noScaleParentMatrix);
		}
		if (isDistanceComplement_) {
			currentWorldPos_ = Vector3{ worldMatrix_.m[3][0], worldMatrix_.m[3][1] ,worldMatrix_.m[3][2] };
			if (firstEmit_) {
				previousWorldPos_ = currentWorldPos_;
				firstEmit_ = false;
			}
		}
		if (grain_.isParent_) {
			isUpDatedMatrix_ = true;
		}


		for (uint32_t i = 0; i < count_; i++) {
			Vector3 posAddSize = Random::GetVector3(
				{ emitSizeMin_.x, emitSizeMax_.x },
				{ emitSizeMin_.y, emitSizeMax_.y },
				{ emitSizeMin_.z, emitSizeMax_.z }
			);


			// 親の回転だけを取り出して適用する
			Matrix4x4 parentRotationOnly = parent_ ? parent_->GetWorldMat() : Matrix4x4::MakeIdentity4x4();
			if (parent_) {
				parentRotationOnly.m[3][0] = 0.0f;
				parentRotationOnly.m[3][1] = 0.0f;
				parentRotationOnly.m[3][2] = 0.0f;
				parentRotationOnly.m[0][3] = 0.0f;
				parentRotationOnly.m[1][3] = 0.0f;
				parentRotationOnly.m[2][3] = 0.0f;
				parentRotationOnly.m[3][3] = 1.0f;
			}
			posAddSize = Transform(posAddSize, parentRotationOnly); // ← 回転だけ適用
			if (!grain_.isParent_) {
				// 最終的な位置はワールド座標の位置を加算
				posAddSize += { worldMatrix_.m[3][0], worldMatrix_.m[3][1], worldMatrix_.m[3][2] };
			}

			if (grain_.speedType_ == static_cast<int>(SpeedType::kCenter)) {
				Vector3 rPos = pos_;
				rPos = Transform(rPos, parentRotationOnly);
				if (grain_.isParent_) {
					grain_.speed_ = (rPos - (posAddSize + rPos)) * grain_.returnPower_;
				} else {
					rPos = pos_ + Vector3{ worldMatrix_.m[3][0], worldMatrix_.m[3][1], worldMatrix_.m[3][2] };
					grain_.speed_ = (rPos - posAddSize) * grain_.returnPower_;
				}
			}

			if (!isDistanceComplement_) {
				if (grain_.isParent_) {
					ParticleManager::ParentEmit(name_, posAddSize, particleRotate_, grain_, para_, 1);
				} else {
					ParticleManager::Emit(name_, posAddSize, particleRotate_, grain_, para_, 1);
				}
			}
		}

		if (isDistanceComplement_) {
			float distanceMoved = (currentWorldPos_ - previousWorldPos_).Length();
			int emitCount = (int)(distanceMoved / 0.05f); // spacing = 理想の間隔

			for (int i = 0; i < emitCount; ++i) {
				float t = (float)i / emitCount;
				Vector3 emitPos = Lerp(previousWorldPos_, currentWorldPos_, t);
				ParticleManager::Emit(name_, emitPos, particleRotate_, grain_, para_, 1);
			}
		}

		time_ = frequencyTime_;
	} else {
		time_--;
	}
}

void ParticleEmitter::Burst() {
	worldMatrix_ = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, pos_);
	if (parent_) {
		const Matrix4x4& parentWorldMatrix = parent_->GetWorldMat();
		worldMatrix_ = Multiply(worldMatrix_, parentWorldMatrix);
	}

	for (uint32_t i = 0; i < count_; i++) {
		Vector3 posAddSize{};
		posAddSize = Random::GetVector3({ emitSizeMin_.x,emitSizeMax_.x }, { emitSizeMin_.y,emitSizeMax_.y }, { emitSizeMin_.z,emitSizeMax_.z });
		posAddSize += {worldMatrix_.m[3][0], worldMatrix_.m[3][1], worldMatrix_.m[3][2]};

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
	worldMatrix_ = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, pos_);
	if (parent_) {
		const Matrix4x4& parentWorldMatrix = parent_->GetWorldMat();
		worldMatrix_ = Multiply(worldMatrix_, parentWorldMatrix);
	}
	return Vector3{ worldMatrix_.m[3][0], worldMatrix_.m[3][1], worldMatrix_.m[3][2] };
}

Matrix4x4 ParticleEmitter::GetParentMatrix() {
	if (parent_) {
		return parent_->GetWorldMat();
	}
	return Matrix4x4::MakeIdentity4x4();
}
