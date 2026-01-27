#include "ParticleEmitter.h"
#include "ParticleManager.h"
#include "Math/Random/Random.h"
#include "ImGuiManager.h"
#include "ParticleGroup/IParticleGroup.h"
#include "Model/Line3dDrawer.h"
#include <iostream>
#include <fstream>

using namespace Core;
using namespace Graphics;
using namespace Math;


ParticleEmitter::ParticleEmitter() {
}

ParticleEmitter::~ParticleEmitter() {
}

void ParticleEmitter::DebugGUI() {
#ifdef _DEBUG
	if (ImGui::CollapsingHeader("エミッター設定")) {
		ImGui::Indent();

		ImGui::Checkbox("Enable Emission", &isEmit_);
		ImGui::SameLine();
		ImGui::Checkbox("Dist Emit", &isDistanceComplement_);
		ImGui::SameLine();
		ImGui::Checkbox("Show Gizmo", &isDrawSize_);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// 座標・発生数・頻度
		ImGui::Text("中心位置:");
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::DragFloat3("##EmitterPos", &pos_.x, 0.01f);

		ImGui::Text("発生数 (per emit):");
		ImGui::SetNextItemWidth(-FLT_MIN);
		int im_Count = int(count_);
		if (ImGui::DragInt("##SpawnCount", &im_Count, 1, 0, 100)) {
			count_ = uint32_t(im_Count);
		}

		ImGui::Text("発生間隔 (sec):");
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::DragFloat("##Frequency", &frequencyTime_, 0.01f, 0.01f, 10.0f);

		ImGui::Text("寿命 (sec):");
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::DragFloat("##Lifetime", &grain_.lifeTime_, 0.1f, 0.1f, 60.0f);

		ImGui::Spacing();

		ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.4f, 1.0f), "パーティクル発生範囲");
		ImGui::Text("発生範囲　最大:");
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::DragFloat3("##SizeMax", &emitSizeMax_.x, 0.01f);

		ImGui::Text("発生範囲　最小:");
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::DragFloat3("##SizeMin", &emitSizeMin_.x, 0.01f);

		ImGui::Unindent();
	}

	ImGui::Spacing();

	// ========== Particle Settings (粒子の挙動) ==========
	if (ImGui::CollapsingHeader("パーティクル設定")) {
		ImGui::Indent();

		if (ImGui::TreeNodeEx("色")) {
			ImGui::Separator();
			ImGui::Text("Color Mode:");
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::Combo("##ColorType", &grain_.colorType_, "Single\0Random\0");

			ImGui::Spacing();

			if (grain_.colorType_ != static_cast<int>(ColorType::kRandom)) {
				ImGui::ColorEdit4("Color", &para_.colorMax.x);
			} else {
				ImGui::ColorEdit4("Max Color", &para_.colorMax.x);
				ImGui::ColorEdit4("Min Color", &para_.colorMin.x);
			}

			ImGui::Spacing();
			ImGui::Checkbox("Fade Alpha (Lifetime)", &grain_.isColorFade_);
			ImGui::Checkbox("Fade In Alpha", &grain_.isColorFadeIn_);

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Text("UV Animation");
			ImGui::Checkbox("Auto UV Scroll", &grain_.isAutoUVMove_);
			if (grain_.isAutoUVMove_) {
				ImGui::Indent();
				ImGui::DragFloat2("Base Speed", &grain_.autoUVSpeed_.x, 0.01f);
				ImGui::DragFloat2("Max Speed", &para_.autoUVMax.x, 0.01f);
				ImGui::DragFloat2("Min Speed", &para_.autoUVMin.x, 0.01f);
				ImGui::Unindent();
			}

			ImGui::TreePop();
		}
		ImGui::Separator();

		if (ImGui::TreeNodeEx("サイズ")) {
			ImGui::Separator();
			ImGui::Text("サイズタイプ:");
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::Combo("##SizeType", &grain_.type_, "一定\0補間\0Sin波\0");

			ImGui::Spacing();

			if (grain_.type_ == static_cast<int>(SizeType::kNormal)) {
				ImGui::DragFloat2("サイズ", &grain_.startSize_.x, 0.01f);
			} else {
				ImGui::DragFloat2("初期サイズ", &grain_.startSize_.x, 0.01f);
				ImGui::DragFloat2("最終サイズ", &grain_.endSize_.x, 0.01f);
				ImGui::Checkbox("Link X/Z Ratio", &grain_.isZandX_);
			}

			ImGui::Spacing();
			ImGui::Checkbox("ランダム幅", &isAddRandomSize_);
			if (isAddRandomSize_) {
				ImGui::Indent();
				ImGui::DragFloat2("最大幅", &addRandomMax_.x, 0.01f);
				ImGui::DragFloat2("最小幅", &addRandomMin_.x, 0.01f);
				ImGui::Unindent();
			}

			ImGui::TreePop();
		}
		ImGui::Separator();

		if (ImGui::TreeNodeEx("回転")) {
			ImGui::Separator();
			ImGui::Text("回転タイプ:");
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::Combo("##RotType", &grain_.rotateType_, "通常\0速度方向\0ランダム\0");

			ImGui::Spacing();
			ImGui::DragFloat3("初期回転角", &particleRotate_.x, 0.1f);
			ImGui::Checkbox("継続回転 有効化", &grain_.isContinuouslyRotate_);

			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.4f, 1.0f), "ビルボード設定");
			ImGui::Checkbox("ビルボード 有効化", &grain_.isBillBoard_);
			if (grain_.isBillBoard_) {
				ImGui::Indent();
				int billPattern = static_cast<int>(grain_.pattern_);
				if (ImGui::RadioButton("XYZ", &billPattern, 0)) grain_.pattern_ = static_cast<BillBoardPattern>(billPattern);
				ImGui::SameLine();
				if (ImGui::RadioButton("X固定", &billPattern, 1)) grain_.pattern_ = static_cast<BillBoardPattern>(billPattern);

				if (ImGui::RadioButton("Y固定", &billPattern, 2)) grain_.pattern_ = static_cast<BillBoardPattern>(billPattern);
				ImGui::SameLine();
				if (ImGui::RadioButton("Z固定", &billPattern, 3)) grain_.pattern_ = static_cast<BillBoardPattern>(billPattern);
				ImGui::SameLine();
				if (ImGui::RadioButton("XY固定", &billPattern, 4)) grain_.pattern_ = static_cast<BillBoardPattern>(billPattern);

				ImGui::Unindent();
			}

			ImGui::TreePop();
		}
		ImGui::Separator();

		if (ImGui::TreeNodeEx("Velocity & Force")) {
			ImGui::Separator();
			ImGui::Text("移動タイプ:");
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::Combo("##VelType", &grain_.speedType_, "一定\0変化\0戻り\0中心へ\0");

			ImGui::Spacing();
			ImGui::DragFloat2("Vel X Range", &para_.speedx.x, 0.01f);
			ImGui::DragFloat2("Vel Y Range", &para_.speedy.x, 0.01f);
			ImGui::DragFloat2("Vel Z Range", &para_.speedz.x, 0.01f);

			if (grain_.speedType_ == static_cast<int>(SpeedType::kReturn) ||
				grain_.speedType_ == static_cast<int>(SpeedType::kCenter)) {
				ImGui::Spacing();
				ImGui::DragFloat("Attraction Force", &grain_.returnPower_, 0.001f);
			}

			if (grain_.speedType_ == static_cast<int>(SpeedType::kReturn)) {
				ImGui::Spacing();
				ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.4f, 1.0f), "Acceleration (Gravity)");
				ImGui::DragFloat3("Accel Vector", &grain_.accele_.x, 0.01f);
			}

			ImGui::TreePop();
		}

		ImGui::Unindent();
	}

	ImGui::Spacing();
	ImGui::Separator();

	if (ImGui::Button("Save Emitter Data", ImVec2(-FLT_MIN, 32))) {
		Save();
	}
#endif // _DEBUG
}

void ParticleEmitter::EmitProgressGUI() {
#ifdef _DEBUG
	float t = 1.0f;
	if (frequencyTime_ != 0.0f) {
		t = 1.0f - (time_ / frequencyTime_);
	}
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "発生まで:");
	ImGui::ProgressBar(t, ImVec2(0, 0));

#endif // _DEBUG
}

void ParticleEmitter::DrawSize() {
#ifdef _DEBUG
	if (isDrawSize_) {


		worldMatrix_ = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, pos_);
		if (parent_ || animeParent_) {
			const Matrix4x4& parentWorldMatrix= animeParent_ ? *animeParent_ : parent_->GetWorldMat();
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
		if (parent_ || animeParent_) {
			const Matrix4x4& parentWorldMatrix = animeParent_ ? *animeParent_ : parent_->GetWorldMat();
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

			if (isAddRandomSize_) {
				para_.addRandomSize.x = Random::GetFloat(addRandomMin_.x, addRandomMax_.x);
				para_.addRandomSize.y = Random::GetFloat(addRandomMin_.y, addRandomMax_.y);
			} else {
				para_.addRandomSize = { 0.0f,0.0f };
			}

			// 親の回転だけを取り出して適用する
			Matrix4x4 parentRotationOnly = Matrix4x4::MakeIdentity4x4();
			if (parent_ || animeParent_) {
				parentRotationOnly = animeParent_ ? *animeParent_ : parent_->GetWorldMat();
				Matrix4x4 parentMat = animeParent_ ? *animeParent_ : parent_->GetWorldMat();

				// 上位3x3から回転だけを取り出す
				Vector3 right = Vector3::Normalize(Vector3(parentMat.m[0][0], parentMat.m[0][1], parentMat.m[0][2]));
				Vector3 up = Vector3::Normalize(Vector3(parentMat.m[1][0], parentMat.m[1][1], parentMat.m[1][2]));
				Vector3 forward = Vector3::Normalize(Vector3(parentMat.m[2][0], parentMat.m[2][1], parentMat.m[2][2]));
				// 再構築（スケール成分なし）
				parentRotationOnly.m[0][0] = right.x;
				parentRotationOnly.m[0][1] = right.y;
				parentRotationOnly.m[0][2] = right.z;

				parentRotationOnly.m[1][0] = up.x;
				parentRotationOnly.m[1][1] = up.y;
				parentRotationOnly.m[1][2] = up.z;

				parentRotationOnly.m[2][0] = forward.x;
				parentRotationOnly.m[2][1] = forward.y;
				parentRotationOnly.m[2][2] = forward.z;
				// 平行移動成分をゼロ
				parentRotationOnly.m[3][0] = 0.0f;
				parentRotationOnly.m[3][1] = 0.0f;
				parentRotationOnly.m[3][2] = 0.0f;
			}
			posAddSize = Transform(posAddSize, parentRotationOnly); // ← 回転だけ適用
			if (!grain_.isParent_) {
				// 最終的な位置はワールド座標の位置を加算
				posAddSize += { worldMatrix_.m[3][0], worldMatrix_.m[3][1], worldMatrix_.m[3][2] };
			}

			if (grain_.speedType_ == static_cast<int>(SpeedType::kCenter)) {
				Vector3 rPos = pos_;
				if (parent_ || animeParent_) {
					rPos = Vector3{ worldMatrix_.m[3][0], worldMatrix_.m[3][1] ,worldMatrix_.m[3][2] };
				}
				if (grain_.isParent_) {
					grain_.speed_ = (rPos - (posAddSize + rPos)) * grain_.returnPower_;
				} else {
					//rPos = pos_ + Vector3{ worldMatrix_.m[3][0], worldMatrix_.m[3][1], worldMatrix_.m[3][2] };
					grain_.speed_ = (rPos - posAddSize) * grain_.returnPower_;
				}
			}

			if (!isDistanceComplement_) {
				if (grain_.isParent_) {
					ParticleManager::ParentEmit(group_, posAddSize, particleRotate_, grain_, para_, 1);
				} else {
					ParticleManager::Emit(group_, posAddSize, particleRotate_, grain_, para_, 1);
				}
			}
		}

		if (isDistanceComplement_) {
			float distanceMoved = (currentWorldPos_ - previousWorldPos_).Length();
			int emitCount = (int)(distanceMoved / 0.05f); // spacing = 理想の間隔

			for (int i = 0; i < emitCount; ++i) {
				float t = (float)i / emitCount;
				Vector3 emitPos = Lerp(previousWorldPos_, currentWorldPos_, t);
				ParticleManager::Emit(group_, emitPos, particleRotate_, grain_, para_, 1);
			}
			if (emitCount == 0) {
				ParticleManager::Emit(group_, currentWorldPos_, particleRotate_, grain_, para_, 1);
			}
		}

		time_ = frequencyTime_;
	} else {
		time_ -= FPSKeeper::DeltaTimeForEffect();
	}
}

void ParticleEmitter::Burst() {
	worldMatrix_ = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, pos_);
	if (parent_ || animeParent_) {
		const Matrix4x4& parentWorldMatrix = animeParent_ ? *animeParent_ : parent_->GetWorldMat();
		worldMatrix_ = Multiply(worldMatrix_, parentWorldMatrix);
	}

	for (uint32_t i = 0; i < count_; i++) {
		Vector3 posAddSize{};
		posAddSize = Random::GetVector3({ emitSizeMin_.x,emitSizeMax_.x }, { emitSizeMin_.y,emitSizeMax_.y }, { emitSizeMin_.z,emitSizeMax_.z });
		posAddSize += {worldMatrix_.m[3][0], worldMatrix_.m[3][1], worldMatrix_.m[3][2]};

		if (grain_.speedType_ == static_cast<int>(SpeedType::kCenter)) {
			grain_.speed_ = (pos_ - posAddSize).Normalize() * grain_.returnPower_;
		}

		ParticleManager::Emit(group_, posAddSize, particleRotate_, grain_, para_, 1);
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

	j["position"]             = { pos_.x, pos_.y, pos_.z };
	j["rotate"]               = { particleRotate_.x,particleRotate_.y,particleRotate_.z };
	j["emitMaxSize"]          = { emitSizeMax_.x,emitSizeMax_.y,emitSizeMax_.z };
	j["emitMinSize"]          = { emitSizeMin_.x,emitSizeMin_.y,emitSizeMin_.z };

	j["count"]                = count_;
	j["frequencyTime"]        = frequencyTime_;

	j["lifeTime"]             = (grain_.lifeTime_);
	j["accele"]               = { grain_.accele_.x,grain_.accele_.y,grain_.accele_.z };
	j["speed"]                = { grain_.speed_.x,grain_.speed_.y,grain_.speed_.z };

	j["grainType"]            = (grain_.type_);
	j["speedType"]            = (grain_.speedType_);
	j["rotateType"]           = (grain_.rotateType_);
	j["colorType"]            = (grain_.colorType_);

	j["returnPower"]          = (grain_.returnPower_);

	j["startSize"]            = { grain_.startSize_.x,grain_.startSize_.y };
	j["endSize"]              = { grain_.endSize_.x,grain_.endSize_.y };

	j["isBillBoard"]          = (grain_.isBillBoard_);
	j["isContinuouslyR_"]     = (grain_.isContinuouslyRotate_);

	j["grainPattern"]         = (static_cast<int>(grain_.pattern_));

	j["isZandX"]              = (grain_.isZandX_);
	j["isAutoUVMove"]         = (grain_.isAutoUVMove_);
	j["autoUVSpeed"]          = { grain_.autoUVSpeed_.x, grain_.autoUVSpeed_.y };

	j["Para_speedx"]          = { para_.speedx.x,para_.speedx.y };
	j["Para_speedy"]          = { para_.speedy.x,para_.speedy.y };
	j["Para_speedz"]          = { para_.speedz.x,para_.speedz.y };

	j["Para_transx"]          = { para_.transx.x,para_.transx.y };
	j["Para_transy"]          = { para_.transy.x,para_.transy.y };
	j["Para_transz"]          = { para_.transz.x,para_.transz.y };

	j["Para_colorMin"]        = { para_.colorMin.x,para_.colorMin.y,para_.colorMin.z,para_.colorMin.w };
	j["Para_colorMax"]        = { para_.colorMax.x,para_.colorMax.y,para_.colorMax.z,para_.colorMax.w };
	
	j["Para_autoUVMax"]       = { para_.autoUVMax.x, para_.autoUVMax.y };
	j["Para_autoUVMin"]       = { para_.autoUVMin.x, para_.autoUVMin.y };

	j["addRandomMax"]         = { addRandomMax_.x, addRandomMax_.y };
	j["addRandomMin"]         = { addRandomMin_.x, addRandomMin_.y };
	j["isAddRandomSize"]      = (isAddRandomSize_);

	j["isDistanceComplement"] = (isDistanceComplement_);

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

	if (j.is_object()) {
		if (j.contains("position"))             pos_ = Vector3(j["position"][0], j["position"][1], j["position"][2]);
		if (j.contains("rotate"))               particleRotate_ = Vector3(j["rotate"][0], j["rotate"][1], j["rotate"][2]);
		if (j.contains("emitMaxSize"))          emitSizeMax_ = Vector3(j["emitMaxSize"][0], j["emitMaxSize"][1], j["emitMaxSize"][2]);
		if (j.contains("emitMinSize"))          emitSizeMin_ = Vector3(j["emitMinSize"][0], j["emitMinSize"][1], j["emitMinSize"][2]);

		if (j.contains("count"))                count_ = j["count"].get<int>();
		if (j.contains("frequencyTime"))        frequencyTime_ = j["frequencyTime"].get<float>();

		if (j.contains("lifeTime"))             grain_.lifeTime_ = j["lifeTime"].get<float>();
		if (j.contains("accele"))               grain_.accele_ = Vector3(j["accele"][0], j["accele"][1], j["accele"][2]);
		if (j.contains("speed"))                grain_.speed_ = Vector3(j["speed"][0], j["speed"][1], j["speed"][2]);

		if (j.contains("grainType"))            grain_.type_ = j["grainType"].get<int>();
		if (j.contains("speedType"))            grain_.speedType_ = j["speedType"].get<int>();
		if (j.contains("rotateType"))           grain_.rotateType_ = j["rotateType"].get<int>();
		if (j.contains("colorType"))            grain_.colorType_ = j["colorType"].get<int>();

		if (j.contains("returnPower"))          grain_.returnPower_ = j["returnPower"].get<float>();

		if (j.contains("startSize"))            grain_.startSize_ = Vector2(j["startSize"][0], j["startSize"][1]);
		if (j.contains("endSize"))              grain_.endSize_ = Vector2(j["endSize"][0], j["endSize"][1]);

		if (j.contains("isBillBoard"))          grain_.isBillBoard_ = j["isBillBoard"].get<bool>();
		if (j.contains("isContinuouslyR_"))     grain_.isContinuouslyRotate_ = j["isContinuouslyR_"].get<bool>();

		if (j.contains("grainPattern"))         grain_.pattern_ = static_cast<BillBoardPattern>(j["grainPattern"].get<int>());

		if (j.contains("isZandX"))              grain_.isZandX_ = j["isZandX"].get<bool>();
		if (j.contains("isAutoUVMove"))         grain_.isAutoUVMove_ = j["isAutoUVMove"].get<bool>();
		if (j.contains("autoUVSpeed"))          grain_.autoUVSpeed_ = Vector2(j["autoUVSpeed"][0], j["autoUVSpeed"][1]);

		if (j.contains("Para_speedx"))          para_.speedx = Vector2(j["Para_speedx"][0], j["Para_speedx"][1]);
		if (j.contains("Para_speedy"))          para_.speedy = Vector2(j["Para_speedy"][0], j["Para_speedy"][1]);
		if (j.contains("Para_speedz"))          para_.speedz = Vector2(j["Para_speedz"][0], j["Para_speedz"][1]);

		if (j.contains("Para_transx"))          para_.transx = Vector2(j["Para_transx"][0], j["Para_transx"][1]);
		if (j.contains("Para_transy"))          para_.transy = Vector2(j["Para_transy"][0], j["Para_transy"][1]);
		if (j.contains("Para_transz"))          para_.transz = Vector2(j["Para_transz"][0], j["Para_transz"][1]);

		if (j.contains("Para_colorMin"))        para_.colorMin = Vector4(j["Para_colorMin"][0], j["Para_colorMin"][1], j["Para_colorMin"][2], j["Para_colorMin"][3]);
		if (j.contains("Para_colorMax"))        para_.colorMax = Vector4(j["Para_colorMax"][0], j["Para_colorMax"][1], j["Para_colorMax"][2], j["Para_colorMax"][3]);

		if (j.contains("Para_autoUVMax"))       para_.autoUVMax = Vector2(j["Para_autoUVMax"][0], j["Para_autoUVMax"][1]);
		if (j.contains("Para_autoUVMin"))       para_.autoUVMin = Vector2(j["Para_autoUVMin"][0], j["Para_autoUVMin"][1]);

		if (j.contains("addRandomMax"))         addRandomMax_ = Vector2(j["addRandomMax"][0], j["addRandomMax"][1]);
		if (j.contains("addRandomMin"))         addRandomMin_ = Vector2(j["addRandomMin"][0], j["addRandomMin"][1]);
		if (j.contains("isAddRandomSize"))      isAddRandomSize_ = j["isAddRandomSize"].get<bool>();

		if (j.contains("isDistanceComplement")) isDistanceComplement_ = j["isDistanceComplement"].get<bool>();
	} else if (j.is_array()) {
		int index = 0;
		pos_ =                Vector3(j[index][0], j[index][1], j[index][2]); index++;
		particleRotate_ =     Vector3(j[index][0], j[index][1], j[index][2]); index++;

		emitSizeMax_ =        Vector3(j[index][0], j[index][1], j[index][2]); index++;
		emitSizeMin_ =        Vector3(j[index][0], j[index][1], j[index][2]); index++;

		count_ =              j[index].get<int>(); index++;
		frequencyTime_ =      j[index].get<float>(); index++;
		grain_.lifeTime_ =    j[index].get<float>(); index++;

		grain_.accele_ =      Vector3(j[index][0], j[index][1], j[index][2]); index++;
		grain_.speed_ =       Vector3(j[index][0], j[index][1], j[index][2]); index++;

		grain_.type_ =        j[index].get<int>(); index++;
		grain_.speedType_ =   j[index].get<int>(); index++;
		grain_.rotateType_ =  j[index].get<int>(); index++;
		grain_.colorType_ =   j[index].get<int>(); index++;

		grain_.returnPower_ = j[index].get<float>(); index++;

		grain_.startSize_ =   Vector2(j[index][0], j[index][1]); index++;
		grain_.endSize_ =     Vector2(j[index][0], j[index][1]); index++;

		grain_.isBillBoard_ = j[index].get<bool>(); index++;

		grain_.pattern_ =     static_cast<BillBoardPattern>(j[index].get<int>()); index++;

		para_.speedx =        Vector2(j[index][0], j[index][1]); index++;
		para_.speedy =        Vector2(j[index][0], j[index][1]); index++;
		para_.speedz =        Vector2(j[index][0], j[index][1]); index++;

		para_.transx =        Vector2(j[index][0], j[index][1]); index++;
		para_.transy =        Vector2(j[index][0], j[index][1]); index++;
		para_.transz =        Vector2(j[index][0], j[index][1]); index++;

		para_.colorMin =      Vector4(j[index][0], j[index][1], j[index][2], j[index][3]); index++;
		para_.colorMax =      Vector4(j[index][0], j[index][1], j[index][2], j[index][3]); index++;
	}
}

Vector3 ParticleEmitter::GetWorldPos() {
	worldMatrix_ = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, pos_);
	if (parent_ || animeParent_) {
		const Matrix4x4& parentWorldMatrix = animeParent_ ? *animeParent_ : parent_->GetWorldMat();
		worldMatrix_ = Multiply(worldMatrix_, parentWorldMatrix);
	}
	return Vector3{ worldMatrix_.m[3][0], worldMatrix_.m[3][1], worldMatrix_.m[3][2] };
}

Matrix4x4 ParticleEmitter::GetParentMatrix() {
	if (parent_) {
		return parent_->GetWorldMat();
	}
	if (animeParent_) {
		return *animeParent_;
	}
	return Matrix4x4::MakeIdentity4x4();
}
