#include "ParticleEmitter.h"
#include "Random.h"
#include "ImGuiManager.h"
#include "Model/Line3dDrawer.h"

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

		ImGui::ColorEdit4("colorMin", &para_.colorMin.X);
		ImGui::ColorEdit4("colorMax", &para_.colorMax.X);

		ImGui::DragFloat("lifetime", &grain.lifeTime_, 0.1f);
		if (ImGui::TreeNode("typeSelect")) {
			ImGui::Combo("sizeType##type", &grain.type, "kNormal\0kShift\0kSin\0");
			ImGui::Combo("speedType##type", &grain.speedType, "kConstancy\0kChange\0");
			ImGui::Combo("colorType##type", &grain.colorType, "kDefault\0kRandom\0");
			ImGui::TreePop();
		}
		ImGui::SeparatorText("size");
		ImGui::DragFloat2("startSize", &grain.startSize.x, 0.01f);
		ImGui::DragFloat2("endSize", &grain.endSize.x, 0.01f);
		ImGui::SeparatorText("randSpeed");
		ImGui::DragFloat2("speedX", &para_.speedx.x, 0.01f);
		ImGui::DragFloat2("speedY", &para_.speedy.x, 0.01f);
		ImGui::DragFloat2("speedZ", &para_.speedz.x, 0.01f);
		ImGui::SeparatorText("billBoard");
		ImGui::Checkbox("BillBoard", &grain.isBillBoard_);
		ImGui::Checkbox("SizeCheck", &isDrawSize_);
		ImGui::TreePop();
	}
	ImGui::End();

}
void ParticleEmitter::DrawSize() {
	if (isDrawSize_) {

		Vector3 points[8];
		points[0] = emitSizeMin;
		points[1] = { emitSizeMax.x,emitSizeMin.y,emitSizeMin.z };
		points[2] = { emitSizeMax.x,emitSizeMin.y,emitSizeMax.z };
		points[3] = { emitSizeMin.x,emitSizeMin.y,emitSizeMax.z };

		points[4] = emitSizeMax;
		points[5] = { emitSizeMin.x,emitSizeMax.y,emitSizeMax.z };
		points[6] = { emitSizeMin.x,emitSizeMax.y,emitSizeMin.z };
		points[7] = { emitSizeMax.x,emitSizeMax.y,emitSizeMin.z };

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

	if (time_ <= 0) {
		for (uint32_t i = 0; i < count; i++) {
			Vector3 posAddSize{};
			posAddSize = Random::GetVector3({ emitSizeMin.x,emitSizeMax.x }, { emitSizeMin.y,emitSizeMax.y }, { emitSizeMin.z,emitSizeMax.z });
			posAddSize += pos;
			ParticleManager::Emit(name, posAddSize, grain, para_, 1);
		}
		time_ = frequencyTime;
	}
	else {
		time_--;
	}

}

void ParticleEmitter::Burst() {
	ParticleManager::Emit(name, pos, grain, para_, count);
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
