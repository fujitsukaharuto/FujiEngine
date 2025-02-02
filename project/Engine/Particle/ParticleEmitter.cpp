#include "ParticleEmitter.h"
#include "Random.h"
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

	ImGui::Begin(name.c_str());
	if (ImGui::TreeNode("emitter")) {
		ImGui::DragFloat3("pos", &pos.x, 0.01f);
		int im_Count = int(count);
		ImGui::DragInt("count", &im_Count, 1, 0, 10);
		count = uint32_t(im_Count);
		ImGui::DragFloat("frenquencyTime", &frequencyTime, 0.1f, 1.0f, 600.0f);
		ImGui::DragFloat3("emitSizeMax", &emitSizeMax.x, 0.01f);
		ImGui::DragFloat3("emitSizeMin", &emitSizeMin.x, 0.01f);
		ImGui::TreePop();
	}
	ImGui::Separator();
	if (ImGui::TreeNode("particle")) {

		ImGui::ColorEdit4("colorMin", &para_.colorMin.x);
		ImGui::ColorEdit4("colorMax", &para_.colorMax.x);

		ImGui::DragFloat("lifetime", &grain.lifeTime_, 0.1f);
		if (ImGui::TreeNode("typeSelect")) {
			ImGui::Combo("sizeType##type", &grain.type, "kNormal\0kShift\0kSin\0");
			ImGui::Combo("speedType##type", &grain.speedType, "kConstancy\0kChange\0kReturn\0");
			ImGui::Combo("rotateType##type", &grain.rotateType, "kUsually\0kVelocityR\0");
			ImGui::Combo("colorType##type", &grain.colorType, "kDefault\0kRandom\0");
			ImGui::TreePop();
		}
		ImGui::SeparatorText("size");
		ImGui::DragFloat2("startSize", &grain.startSize.x, 0.01f);
		ImGui::DragFloat2("endSize", &grain.endSize.x, 0.01f);
		ImGui::SeparatorText("rotate");
		ImGui::DragFloat3("rotate", &particleRotate.x, 0.01f);
		ImGui::SeparatorText("randSpeed");
		ImGui::DragFloat2("speedX", &para_.speedx.x, 0.01f);
		ImGui::DragFloat2("speedY", &para_.speedy.x, 0.01f);
		ImGui::DragFloat2("speedZ", &para_.speedz.x, 0.01f);
		if (grain.speedType == SpeedType::kReturn) {
			ImGui::DragFloat("returnPower", &grain.returnPower_, 0.001f);
		}
		ImGui::SeparatorText("accele");
		ImGui::DragFloat3("accele", &grain.accele.x, 0.01f);
		ImGui::SeparatorText("billBoard");
		ImGui::Checkbox("BillBoard", &grain.isBillBoard_);
		if (grain.isBillBoard_) {
			int billPattern = static_cast<int>(grain.pattern_);
			ImGui::RadioButton("XYZ", &billPattern, 0); ImGui::SameLine();
			ImGui::RadioButton("X", &billPattern, 1); ImGui::SameLine();
			ImGui::RadioButton("Y", &billPattern, 2); ImGui::SameLine();
			ImGui::RadioButton("Z", &billPattern, 3);
			grain.pattern_ = static_cast<BillBoardPattern>(billPattern);
		}
		ImGui::Checkbox("SizeCheck", &isDrawSize_);
		ImGui::TreePop();
	}
	if (ImGui::Button("save")) {
		Save();
	}
	ImGui::End();

}
void ParticleEmitter::DrawSize() {
	if (isDrawSize_) {

		Matrix4x4 worldMatrix = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, pos);
		if (parent_) {
			const Matrix4x4& parentWorldMatrix = parent_->GetWorldMat();
			worldMatrix = Multiply(worldMatrix, parentWorldMatrix);
		}

		Vector3 points[8];
		points[0] = emitSizeMin;
		points[0] += {worldMatrix.m[3][0], worldMatrix.m[3][1], worldMatrix.m[3][2]};
		points[1] = { emitSizeMax.x,emitSizeMin.y,emitSizeMin.z };
		points[1] += {worldMatrix.m[3][0], worldMatrix.m[3][1], worldMatrix.m[3][2]};
		points[2] = { emitSizeMax.x,emitSizeMin.y,emitSizeMax.z };
		points[2] += {worldMatrix.m[3][0], worldMatrix.m[3][1], worldMatrix.m[3][2]};
		points[3] = { emitSizeMin.x,emitSizeMin.y,emitSizeMax.z };
		points[3] += {worldMatrix.m[3][0], worldMatrix.m[3][1], worldMatrix.m[3][2]};

		points[4] = emitSizeMax;
		points[4] += {worldMatrix.m[3][0], worldMatrix.m[3][1], worldMatrix.m[3][2]};
		points[5] = { emitSizeMin.x,emitSizeMax.y,emitSizeMax.z };
		points[5] += {worldMatrix.m[3][0], worldMatrix.m[3][1], worldMatrix.m[3][2]};
		points[6] = { emitSizeMin.x,emitSizeMax.y,emitSizeMin.z };
		points[6] += {worldMatrix.m[3][0], worldMatrix.m[3][1], worldMatrix.m[3][2]};
		points[7] = { emitSizeMax.x,emitSizeMax.y,emitSizeMin.z };
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

	Matrix4x4 worldMatrix = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, pos);
	if (parent_) {
		const Matrix4x4& parentWorldMatrix = parent_->GetWorldMat();
		worldMatrix = Multiply(worldMatrix, parentWorldMatrix);
	}

	if (time_ <= 0) {
		for (uint32_t i = 0; i < count; i++) {
			Vector3 posAddSize{};
			posAddSize = Random::GetVector3({ emitSizeMin.x,emitSizeMax.x }, { emitSizeMin.y,emitSizeMax.y }, { emitSizeMin.z,emitSizeMax.z });
			posAddSize += {worldMatrix.m[3][0], worldMatrix.m[3][1], worldMatrix.m[3][2]};

			ParticleManager::Emit(name, posAddSize, particleRotate, grain, para_, 1);
		}
		time_ = frequencyTime;
	}
	else {
		time_--;
	}

}

void ParticleEmitter::Burst() {

	Matrix4x4 worldMatrix = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, pos);
	if (parent_) {
		const Matrix4x4& parentWorldMatrix = parent_->GetWorldMat();
		worldMatrix = Multiply(worldMatrix, parentWorldMatrix);
	}

	for (uint32_t i = 0; i < count; i++) {
		Vector3 posAddSize{};
		posAddSize = Random::GetVector3({ emitSizeMin.x,emitSizeMax.x }, { emitSizeMin.y,emitSizeMax.y }, { emitSizeMin.z,emitSizeMax.z });
		posAddSize += {worldMatrix.m[3][0], worldMatrix.m[3][1], worldMatrix.m[3][2]};

		ParticleManager::Emit(name, posAddSize, particleRotate, grain, para_, 1);
	}

}

void ParticleEmitter::BurstAnime() {

	ParticleManager::EmitAnime(name, pos, animeData, para_, count);

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

	j.push_back(json::array({ pos.x,pos.y,pos.z }));
	j.push_back(json::array({ particleRotate.x,particleRotate.y,particleRotate.z }));
	j.push_back(json::array({ emitSizeMax.x,emitSizeMax.y,emitSizeMax.z }));
	j.push_back(json::array({ emitSizeMin.x,emitSizeMin.y,emitSizeMin.z }));

	j.push_back(count);
	j.push_back(frequencyTime);

	j.push_back(grain.lifeTime_);
	j.push_back(json::array({ grain.accele.x,grain.accele.y,grain.accele.z }));
	j.push_back(json::array({ grain.speed.x,grain.speed.y,grain.speed.z }));

	j.push_back(grain.type);
	j.push_back(grain.speedType);
	j.push_back(grain.rotateType);
	j.push_back(grain.colorType);

	j.push_back(grain.returnPower_);

	j.push_back(json::array({ grain.startSize.x,grain.startSize.y }));
	j.push_back(json::array({ grain.endSize.x,grain.endSize.y }));

	j.push_back(grain.isBillBoard_);

	j.push_back(static_cast<int>(grain.pattern_));

	j.push_back(json::array({ para_.speedx.x,para_.speedx.y }));
	j.push_back(json::array({ para_.speedy.x,para_.speedy.y }));
	j.push_back(json::array({ para_.speedz.x,para_.speedz.y }));

	j.push_back(json::array({ para_.transx.x,para_.transx.y }));
	j.push_back(json::array({ para_.transy.x,para_.transy.y }));
	j.push_back(json::array({ para_.transz.x,para_.transz.y }));

	j.push_back(json::array({ para_.colorMin.x,para_.colorMin.y,para_.colorMin.z,para_.colorMin.w }));
	j.push_back(json::array({ para_.colorMax.x,para_.colorMax.y,para_.colorMax.z,para_.colorMax.w }));


	std::ofstream file(kDirectoryPath + name + ".json");
	if (file.is_open()) {
		file << j.dump(4);
		file.close();
	}

}

void ParticleEmitter::Load(const std::string& filename) {

	std::ifstream file(kDirectoryPath + filename + ".json");
	if (!file.is_open()) {
		return;
	}

	json j;
	file >> j;
	file.close();

	// データをロードしてメンバーに復元
	int index = 0;
	pos = Vector3(j[index][0], j[index][1], j[index][2]);
	index++;
	particleRotate = Vector3(j[index][0], j[index][1], j[index][2]);
	index++;

	emitSizeMax = Vector3(j[index][0], j[index][1], j[index][2]);
	index++;
	emitSizeMin = Vector3(j[index][0], j[index][1], j[index][2]);
	index++;

	count = j[index].get<int>();
	index++;
	frequencyTime = j[index].get<float>();
	index++;
	grain.lifeTime_ = j[index].get<float>();
	index++;

	grain.accele = Vector3(j[index][0], j[index][1], j[index][2]);
	index++;
	grain.speed = Vector3(j[index][0], j[index][1], j[index][2]);
	index++;

	grain.type = j[index].get<int>();
	index++;
	grain.speedType = j[index].get<int>();
	index++;
	grain.rotateType = j[index].get<int>();
	index++;
	grain.colorType = j[index].get<int>();
	index++;

	grain.returnPower_ = j[index].get<float>();
	index++;

	grain.startSize = Vector2(j[index][0], j[index][1]);
	index++;
	grain.endSize = Vector2(j[index][0], j[index][1]);
	index++;

	grain.isBillBoard_ = j[index].get<bool>();
	index++;

	grain.pattern_ = static_cast<BillBoardPattern>(j[index].get<int>());
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
	Matrix4x4 worldMatrix = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, pos);
	if (parent_) {
		const Matrix4x4& parentWorldMatrix = parent_->GetWorldMat();
		worldMatrix = Multiply(worldMatrix, parentWorldMatrix);
	}
	return Vector3{ worldMatrix.m[3][0], worldMatrix.m[3][1], worldMatrix.m[3][2] };
}
