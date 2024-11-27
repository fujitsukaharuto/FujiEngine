#include "ParticleEmitter.h"
#include "Random.h"
#include "ImGuiManager.h"

ParticleEmitter::ParticleEmitter() {
}

ParticleEmitter::~ParticleEmitter() {
}

#ifdef _DEBUG
void ParticleEmitter::DebugGUI() {

	ImGui::Begin(name.c_str());
	if (ImGui::TreeNode("emitter")) {
		ImGui::DragFloat3("pos", &pos.x, 0.01f);
		ImGui::DragFloat("frenquencyTime", &frequencyTime, 0.1f);
		ImGui::TreePop();
	}
	ImGui::Separator();
	if (ImGui::TreeNode("particle")) {



		ImGui::TreePop();
	}
	ImGui::End();

}
#endif // _DEBUG

void ParticleEmitter::Emit() {

	if (time_ <= 0) {
		ParticleManager::Emit(name, pos, grain, para_, count);
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
