#include "ParticleManager.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/SRVManager.h"
#include "Engine/Camera/CameraManager.h"
#include "Particle.h"
#include "Math/Random/Random.h"
#include "Engine/DX/FPSKeeper.h"
#include "Engine/Model/ModelManager.h"
#include "ImGuiManager/ImGuiManager.h"
#include "Engine/Editor/JsonSerializer.h"
#include "Engine/DX/FrameCount.h"
#include <fstream>
#include <filesystem>

using namespace Core;
using namespace Graphics;
using namespace Math;


ParticleManager::ParticleManager() {
}

ParticleManager::~ParticleManager() {
}

ParticleManager* ParticleManager::GetInstance() {
	static ParticleManager instance;
	return &instance;
}

void ParticleManager::Initialize(DXCom* pDxcom, SRVManager* srvManager) {
	dxcommon_ = pDxcom;
	srvManager_ = srvManager;
	this->camera_ = CameraManager::GetInstance()->GetCamera();

	InitPlaneVertex();
	InitRingVertex();
	InitSphereVertex();
	InitCylinderVertex();
	InitLighningVertex();

	gpuParticleSystem_ = std::make_unique<GPUParticleSystem>();
	gpuParticleSystem_->Initialize(pDxcom, srvManager);
	
	LoadCSEmitterFileDir();
}

void ParticleManager::Finalize() {
#ifdef _DEBUG
	selectParticleGroup_ = nullptr;
#endif // _DEBUG

	dxcommon_ = nullptr;
	srvManager_ = nullptr;
	camera_ = nullptr;
	particleGroups_.clear();
	parentParticleGroups_.clear();
	for (auto& groupPair : animeGroups_) {

		groupPair.second->lifeTime.clear();
		groupPair.second->startLifeTime_.clear();
		groupPair.second->isLive_.clear();
		groupPair.second->accele.clear();
		groupPair.second->speed.clear();
		groupPair.second.reset();
	}
	animeGroups_.clear();

	plane_.vBuffer.Reset();
	plane_.iBuffer.Reset();
	ring_.vBuffer.Reset();
	ring_.iBuffer.Reset();
	sphere_.vBuffer.Reset();
	sphere_.iBuffer.Reset();
	cylinder_.vBuffer.Reset();
	cylinder_.iBuffer.Reset();
	lightning_.reset();

	gpuParticleSystem_->Finalize();
}

void ParticleManager::Update() {
	Matrix4x4 billboardMatrix = MakeIdentity4x4();
	if (camera_) {
		const Matrix4x4& viewMatrix = camera_->GetViewMatrix();

		billboardMatrix.m[0][0] = viewMatrix.m[0][0];
		billboardMatrix.m[0][1] = viewMatrix.m[1][0];
		billboardMatrix.m[0][2] = viewMatrix.m[2][0];

		billboardMatrix.m[1][0] = viewMatrix.m[0][1];
		billboardMatrix.m[1][1] = viewMatrix.m[1][1];
		billboardMatrix.m[1][2] = viewMatrix.m[2][1];

		billboardMatrix.m[2][0] = viewMatrix.m[0][2];
		billboardMatrix.m[2][1] = viewMatrix.m[1][2];
		billboardMatrix.m[2][2] = viewMatrix.m[2][2];
	}

	gpuParticleSystem_->Update(billboardMatrix);

	UpdateParticleGroup(billboardMatrix);
	UpdateParentParticleGroup(billboardMatrix);
	UpdateAnimeGroup(billboardMatrix);
}

void ParticleManager::Draw() {
	gpuParticleSystem_->Draw(plane_.vbView, plane_.ibView);

	dxcommon_->GetDXCommand()->SetViewAndscissor();
	dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Normal);
	dxcommon_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	for (auto& groupPair : animeGroups_) {
		AnimeGroup* group = groupPair.second.get();
		for (int i = 0; i < group->objects_.size(); i++) {
			if (!group->isLive_[i]) continue;
			group->objects_[i]->AnimeDraw();
		}
	}

	dxcommon_->GetDXCommand()->SetViewAndscissor();
	dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Particle);
	preType_ = BlendType::ADD;
	dxcommon_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &plane_.vbView);
	dxcommon_->GetCommandList()->IASetIndexBuffer(&plane_.ibView);
	DrawParticleGroup();
	DrawParentParticleGroup();
}

void ParticleManager::ParticleDebugGUI() {
#ifdef _DEBUG
	ImGui::Begin("Particle Editor", nullptr, ImGuiWindowFlags_NoCollapse);

	// テーブルフラグ設定：
	// Resizable: 境界線をドラッグ可能にする
	// BordersInnerV: 列の間に線（リサイズハンドル）を表示
	static float leftPaneWidth = 260.0f;
	ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV;

	if (ImGui::BeginTable("EditorMainSplit", 2, tableFlags)) {
		// --- カラム設定 ---
		// 左カラム: 初期幅固定
		ImGui::TableSetupColumn("Hierarchy", ImGuiTableColumnFlags_WidthFixed, leftPaneWidth);
		// 右カラム: 残りの領域すべて
		ImGui::TableSetupColumn("Inspector", ImGuiTableColumnFlags_WidthStretch);

		// =============================================
		// [左側] Hierarchy Pane
		// =============================================
		ImGui::TableNextColumn();

		ImGui::BeginChild("LeftPane", ImVec2(-FLT_MIN, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
		{
			ImGui::SeparatorText("Particle Groups");
			if (ImGui::BeginListBox("##GroupList", ImVec2(-FLT_MIN, -120))) {
				int idx = 0;
				for (const auto& pair : particleGroups_) {
					// 選択状態の判定
					const bool is_selected = (selectParticleGroup_ == pair.second.get());

					if (ImGui::Selectable(pair.first.c_str(), is_selected)) {
						selectParticleGroup_ = pair.second.get();
						currentKey_ = pair.first;
						currentIndex_ = idx;
					}

					// 初期フォーカス設定
					if (is_selected) {
						ImGui::SetItemDefaultFocus();
					}
					idx++;
				}
				ImGui::EndListBox();
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// グローバル操作エリア
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Global Controls");
			if (ImGui::Button("Reset All Timers", ImVec2(-FLT_MIN, 0))) {
				for (auto& groupPair : particleGroups_) {
					groupPair.second->emitter_.TimeReset();
				}
			}
			if (ImGui::Button(isStopped_ ? "Start" : "Stop", ImVec2(80, 0))) {
				isStopped_ ? FPSKeeper::SetUnStopped() : FPSKeeper::SetStopped();
				isStopped_ = !isStopped_;
			}

			ImGui::SameLine();
			// コマ送り（>>）
			if (FPSKeeper::GetFrameByFrame()) {
				FPSKeeper::SetFrameByFrame(false);
			}
			if (ImGui::Button(">>", ImVec2(40, 0))) {
				FPSKeeper::SetFrameByFrame(true);
			}

			ImGui::Spacing();

			// Emit ON/OFF ツリー
			if (ImGui::TreeNodeEx("Emit Toggle Tree", ImGuiTreeNodeFlags_Framed)) {
				static ParticleGroupSelector selector;

				if (selector.items[0].empty() && selector.items[1].empty()) {
					for (const auto& [name, group] : particleGroups_) {
						if (group->emitter_.isEmit_)
							selector.items[1].push_back(name);
						else
							selector.items[0].push_back(name);
					}
				}

				selector.Show([&](const std::string& name, bool emit) {
					auto it = particleGroups_.find(name);
					if (it != particleGroups_.end()) {
						it->second->emitter_.isEmit_ = emit;
					}
					});

				ImGui::TreePop();
			}
		}
		ImGui::EndChild(); // LeftPane End


		// =============================================
		// [右側] Inspector Pane
		// =============================================
		ImGui::TableNextColumn();

		// 右側も独立してスクロール可能にする
		ImGui::BeginChild("RightPane", ImVec2(-FLT_MIN, 0), false);
		{
			if (selectParticleGroup_) {
				ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", currentKey_.c_str());
				ImGui::Separator();
				ImGui::Spacing();

				// 横並びレイアウトでスペース効率化
				ImGui::BeginGroup();
				{
					// 画像表示
					ImGui::Image(
						(ImTextureID)TextureManager::GetInstance()->GetTexture(
							selectParticleGroup_->material_.GetPathName().c_str())->gpuHandle.ptr,
						ImVec2(80, 80),
						ImVec2(0, 0), ImVec2(1, 1),
						ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.5f) // 枠線をつける
					);
				}
				ImGui::EndGroup();
				ImGui::SameLine();
				ImGui::BeginGroup();
				{
					ImGui::Text("Material:");
					ParticleTexurePopUp();
					ImGui::Spacing();
					ImGui::TextDisabled("Path: %s", selectParticleGroup_->material_.GetPathName().c_str());
				}
				ImGui::EndGroup();

				ImGui::Spacing();
				ImGui::Separator();

				if (ImGui::CollapsingHeader("Basic Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
					ImGui::Indent();

					// Shape Type
					int shapeType = static_cast<int>(selectParticleGroup_->shapeType_);
					ImGui::Text("形状");
					ImGui::SetNextItemWidth(-FLT_MIN);
					if (ImGui::Combo("##ShapeType", &shapeType,
						"平面\0リング\0球\0トーラス\0円柱\0コーン\0三角形\0キューブ\0雷\0")) {
						selectParticleGroup_->shapeType_ = static_cast<ShapeType>(shapeType);
					}

					ImGui::Spacing();

					// Blend Mode
					int blendType = static_cast<int>(selectParticleGroup_->type_);
					ImGui::Text("Blend Mode");
					ImGui::SetNextItemWidth(-FLT_MIN);
					if (ImGui::Combo("##BlendType", &blendType,
						"アルファ\0加算\0減算\0スクリーン\0乗算\0ソフト加算\0プリマルチプライド\0")) {
						selectParticleGroup_->type_ = static_cast<BlendType>(blendType);
					}

					ImGui::Spacing();

					// Counts
					int maxCount = static_cast<uint32_t>(selectParticleGroup_->instanceCount_);
					ImGui::Text("最大パーティクル数");
					ImGui::SetNextItemWidth(-FLT_MIN);
					if (ImGui::DragInt("##MaxCount", &maxCount, 10, 1, 10000)) {
						selectParticleGroup_->instanceCount_ = static_cast<uint32_t>(maxCount);
					}

					ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "生存数: %d", int(selectParticleGroup_->drawCount_));

					selectParticleGroup_->emitter_.EmitProgressGUI();

					ImGui::Spacing();
					if (ImGui::Button("Save Group Config", ImVec2(-FLT_MIN, 0))) {
						SaveGroupData();
					}

					ImGui::Unindent();
				}

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				selectParticleGroup_->emitter_.DebugGUI();
				ImGui::Dummy(ImVec2(0, 50));
			} else {
				// 非選択時の表示
				float winWidth = ImGui::GetContentRegionAvail().x;
				float winHeight = ImGui::GetContentRegionAvail().y;
				ImGui::SetCursorPos(ImVec2(winWidth * 0.5f - 60, winHeight * 0.5f));
				ImGui::TextDisabled("Select a Particle Group");
			}
		}
		ImGui::EndChild(); // RightPane End

		ImGui::EndTable();
	}


	ImGui::End();
#endif // _DEBUG
}

void ParticleManager::ParticleCSDebugGUI() {
#ifdef _DEBUG
	gpuParticleSystem_->ParticleCSDebugGUI();
	gpuParticleSystem_->ParticleTexCSDebugGUI();
	gpuParticleSystem_->ParticleSurfaceCSDebugGUI();
#endif // _DEBUG
}

void ParticleManager::SelectParticleUpdate() {
#ifdef _DEBUG
	if (selectParticleGroup_) {
		selectParticleGroup_->emitter_.Emit();
	}
#endif // _DEBUG
}

void ParticleManager::SelectEmitterSizeDraw() {
#ifdef _DEBUG
	if (selectParticleGroup_) {
		selectParticleGroup_->emitter_.DrawSize();
	}
#endif // _DEBUG
}

void ParticleManager::CreateParticleGroup(const std::string& name, const std::string& fileName, uint32_t count, ShapeType shape, BlendType blendType) {
	ParticleManager* instance = GetInstance();
	auto iterator = instance->particleGroups_.find(name);
	if (iterator != instance->particleGroups_.end()) {
		return;
	}

	instance->JsonCheckForGroup(name, fileName, count, shape, blendType);
}

void ParticleManager::CreateParentParticleGroup(const std::string& name, const std::string& fileName, uint32_t count, ShapeType shape, BlendType blendType) {
	ParticleManager* instance = GetInstance();
	auto iterator = instance->parentParticleGroups_.find(name);
	if (iterator != instance->parentParticleGroups_.end()) {
		return;
	}

	instance->JsonCheckForGroup(name, fileName, count, shape, blendType, true);
}

void ParticleManager::CreateAnimeGroup(const std::string& name, const std::string& fileName) {
	ParticleManager* instance = GetInstance();
	auto iterator = instance->animeGroups_.find(name);
	if (iterator != instance->animeGroups_.end()) {
		return;
	}

	std::unique_ptr<AnimeGroup> newGroup;
	newGroup = std::make_unique<AnimeGroup>();

	newGroup->farst = fileName;
	TextureManager::GetInstance()->LoadTexture(fileName);
	for (int i = 0; i < 6; i++) {
		std::unique_ptr<Object3d> newobj = std::make_unique<Object3d>();
		newobj->Create("plane.obj");
		newobj->SetTexture(fileName);


		newGroup->objects_.push_back(std::move(newobj));
		newGroup->lifeTime.push_back(0.0f);
		newGroup->animeTime.push_back(0.0f);
		newGroup->startLifeTime_.push_back(0.0f);
		newGroup->isLive_.push_back(false);
		newGroup->accele.push_back({ 0.0f,0.0f,0.0f });
		newGroup->speed.push_back({ 0.0f,0.0f,0.0f });

	}

	instance->animeGroups_.insert(std::make_pair(name, std::move(newGroup)));
}

void ParticleManager::Load(ParticleEmitter& emit, const std::string& name) {
	ParticleManager* instance = GetInstance();
	emit.name_ = name;

	auto iterator = instance->particleGroups_.find(name);
	if (iterator != instance->particleGroups_.end()) {
		ParticleGroup* group = iterator->second.get();
		emit.pos_                         = group->emitter_.pos_;
		emit.particleRotate_              = group->emitter_.particleRotate_;
		emit.emitSizeMax_                 = group->emitter_.emitSizeMax_;
		emit.emitSizeMin_                 = group->emitter_.emitSizeMin_;
		emit.count_                       = group->emitter_.count_;
		emit.frequencyTime_               = group->emitter_.frequencyTime_;
		emit.isAddRandomSize_             = group->emitter_.isAddRandomSize_;
		emit.addRandomMax_                = group->emitter_.addRandomMax_;
		emit.addRandomMin_                = group->emitter_.addRandomMin_;
		emit.isDistanceComplement_        = group->emitter_.isDistanceComplement_;
		emit.grain_.lifeTime_             = group->emitter_.grain_.lifeTime_;
		emit.grain_.type_                 = group->emitter_.grain_.type_;
		emit.grain_.startSize_            = group->emitter_.grain_.startSize_;
		emit.grain_.endSize_              = group->emitter_.grain_.endSize_;
		emit.grain_.isZandX_              = group->emitter_.grain_.isZandX_;
		emit.grain_.speedType_            = group->emitter_.grain_.speedType_;
		emit.grain_.accele_               = group->emitter_.grain_.accele_;
		emit.grain_.speed_                = group->emitter_.grain_.speed_;
		emit.grain_.returnPower_          = group->emitter_.grain_.returnPower_;
		emit.grain_.rotateType_           = group->emitter_.grain_.rotateType_;
		emit.grain_.isContinuouslyRotate_ = group->emitter_.grain_.isContinuouslyRotate_;
		emit.grain_.isColorFade_          = group->emitter_.grain_.isColorFade_;
		emit.grain_.isColorFadeIn_        = group->emitter_.grain_.isColorFadeIn_;
		emit.grain_.isAutoUVMove_         = group->emitter_.grain_.isAutoUVMove_;
		emit.grain_.autoUVSpeed_          = group->emitter_.grain_.autoUVSpeed_;
		emit.grain_.colorType_            = group->emitter_.grain_.colorType_;
		emit.grain_.isBillBoard_          = group->emitter_.grain_.isBillBoard_;
		emit.grain_.pattern_              = group->emitter_.grain_.pattern_;
		emit.para_                        = group->emitter_.para_;
		emit.SetGroup(group);
	} else {
		return;
	}
}

void ParticleManager::LoadParentGroup(ParticleEmitter*& emit, const std::string& name) {
	ParticleManager* instance = GetInstance();

	auto iterator = instance->parentParticleGroups_.find(name);
	if (iterator != instance->parentParticleGroups_.end()) {
		ParentParticleGroup* group = iterator->second.get();
		emit = group->emitter_.get();
		emit->grain_.isParent_ = true;
		emit->SetGroup(group);

	} else {
		return;
	}
}

void ParticleManager::LoadAllFileData() {
	std::string path = "resource/ParticleGroups/";
	if (!std::filesystem::exists(path)) return;
	//std::string path = "resource/ParticleGroups/" + name + ".json";

	for (const auto& entry : std::filesystem::directory_iterator(path)) {
		if (entry.is_regular_file()) {
			const auto& filePath = entry.path();
			if (filePath.extension() == ".json") {
				// .json を除いたファイル名
				CreateParticleGroup(filePath.stem().string());
			}
		}
	}
}

void ParticleManager::Emit(IParticleGroup* group, const Vector3& pos, const Vector3& rotate, const Particle& grain, const RandomParametor& para, uint32_t count) {
	if (group) {
		uint32_t newCount = 0;
		for (auto& particle : group->particles_) {

			if (group->InitEmitParticle(particle, pos, rotate, grain, para)) {
				newCount++;
			}
			if (newCount == count) {
				return;
			}
		}
	} else {
		return;
	}
}

void ParticleManager::ParentEmit(IParticleGroup* group, const Vector3& pos, const Vector3& rotate, const Particle& grain, const RandomParametor& para, uint32_t count) {
	if (group) {
		uint32_t newCount = 0;
		for (auto& particle : group->particles_) {

			if (group->InitEmitParticle(particle, pos, rotate, grain, para)) {
				newCount++;
			}
			if (newCount == count) {
				return;
			}
		}
	} else {
		return;
	}
}

void ParticleManager::EmitAnime(const std::string& name, const Vector3& pos, const AnimeData& data, const RandomParametor& para, uint32_t count) {
	ParticleManager* instance = GetInstance();

	auto iterator = instance->animeGroups_.find(name);
	if (iterator != instance->animeGroups_.end()) {
		uint32_t newCount = 0;


		AnimeGroup* group = iterator->second.get();
		group->speedType = data.speedType;
		group->type = data.type;
		group->startSize = data.startSize;
		group->endSize = data.endSize;
		for (int i = 0; i < group->objects_.size(); i++) {

			if (!group->isLive_[i]) {
				group->objects_[i]->transform.translate = Random::GetVector3(para.transx, para.transy, para.transz);
				group->objects_[i]->transform.translate += pos;
				group->speed[i] = Random::GetVector3(para.speedx, para.speedy, para.speedz);
				group->lifeTime[i] = data.lifeTime;
				group->startLifeTime_[i] = group->lifeTime[i];
				group->animeTime[i] = 0.0f;

				SpeedType type = SpeedType(group->speedType);
				switch (type) {
				case SpeedType::kConstancy:
					group->accele[i] = Vector3{ 0.0f,0.0f,0.0f };
					break;
				case SpeedType::kChange:
					group->accele[i] = (group->speed[i]) * -0.05f;
					break;
				}

				group->objects_[i]->SetTexture(group->farst);
				group->isLive_[i] = true;
				newCount++;
			}
			if (newCount == count) {
				return;
			}
		}
	} else {
		return;
	}
}

void ParticleManager::AddAnime(const std::string& name, const std::string& fileName, float animeChangeTime) {
	ParticleManager* instance = GetInstance();

	auto iterator = instance->animeGroups_.find(name);
	if (iterator != instance->animeGroups_.end()) {

		AnimeGroup* group = iterator->second.get();
		TextureManager::GetInstance()->LoadTexture(fileName);
		group->anime_.insert(std::make_pair(fileName, animeChangeTime));

	} else {
		return;
	}
}

void ParticleManager::ParentReset() {
	ParticleManager* instance = GetInstance();
	for (auto& groupPair : instance->parentParticleGroups_) {
		ParentParticleGroup* group = groupPair.second.get();
		if (group->emitter_->HaveParent()) {
			group->emitter_->SetParent(nullptr);
		}
	}
}

IGPUEmitter& ParticleManager::GetParticleCSEmitter(int index) {
	ParticleManager* instance = GetInstance();
	return instance->gpuParticleSystem_->GetParticleCSEmitter(index);
}

SphereEmitter& ParticleManager::GetSphereEmitter(int index) {
	ParticleManager* instance = GetInstance();
	return instance->gpuParticleSystem_->GetSphereEmitter(index);
}

TextureBasedEmitter& ParticleManager::GetParticleCSEmitterTexture(int index) {
	ParticleManager* instance = GetInstance();
	return instance->gpuParticleSystem_->GetParticleCSEmitterTexture(index);
}

MeshSurefaceEmitter& ParticleManager::GetParticleCSEmitterSurface(int index) {
	ParticleManager* instance = GetInstance();
	return instance->gpuParticleSystem_->GetParticleCSEmitterSurface(index);
}

void ParticleManager::SetIsStopped(bool flag) {
	ParticleManager* instance = GetInstance();
	instance->isStopped_ = flag;
}

int ParticleManager::InitGPUEmitter(int returnMod) {
	return gpuParticleSystem_->InitGPUEmitter(returnMod);
}

int ParticleManager::InitGPUEmitterTexture(const std::string& fileName) {
	return gpuParticleSystem_->InitGPUEmitterTexture(fileName);
}

int ParticleManager::InitGPUEmitterSurface(const std::string& fileName) {
	return gpuParticleSystem_->InitGPUEmitterSurface(fileName);
}

void ParticleManager::ResetCSEmitters() {
	gpuParticleSystem_->ResetEmitters();
}

void ParticleManager::InitDefoultCSEmitter() {
	gpuParticleSystem_->InitDefoultEmitter();
}

void ParticleManager::LoadCSEmitterFileDir() {
	csEmitterFileNames_.clear();
	const std::filesystem::path dirPath = "resource/EmitterSaveFile/GPUEmitter";
	if (!std::filesystem::exists(dirPath) || !std::filesystem::is_directory(dirPath)) {
		return;
	}

	for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
		if (!entry.is_regular_file()) {
			continue;
		}
		const std::filesystem::path& path = entry.path();

		// 拡張子が .json のみ
		if (path.extension() == ".json") {
			// 拡張子なしのファイル名を取得
			csEmitterFileNames_.push_back(
				path.stem().string()
			);
		}
	}
}

void ParticleManager::InternalCreateParticleGroup(const std::string& name, const std::string& fileName, uint32_t count, ShapeType shape, BlendType blendType, bool isParent) {
	if (isParent) {
		auto iterator = parentParticleGroups_.find(name);
		if (iterator != parentParticleGroups_.end()) {
			return;
		}

		auto newGroup = std::make_unique<ParentParticleGroup>();
		newGroup->emitter_ = std::make_unique<ParticleEmitter>();
		newGroup->shapeType_ = shape;
		newGroup->emitter_->name_ = name;
		newGroup->emitter_->Load(name);

		newGroup->instanceCount_ = count;
		uint32_t max = newGroup->instanceCount_;
		for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
			newGroup->instancing_[i] = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), (sizeof(TransformationParticleMatrix) * newGroup->instanceCount_));
			newGroup->instancing_[i]->Map(0, nullptr, reinterpret_cast<void**>(&newGroup->instancingDataGPU_[i]));
			for (uint32_t index = 0; index < max; ++index) {
				newGroup->instancingDataGPU_[i][index].WVP = MakeIdentity4x4();
				newGroup->instancingDataGPU_[i][index].World = MakeIdentity4x4();
			}
		}
		newGroup->material_.SetTextureNamePath(fileName);
		newGroup->material_.CreateMaterial();
		for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
			newGroup->srvIndex_[i] = srvManager_->Allocate();
			srvManager_->CreateStructuredSRV(newGroup->srvIndex_[i], newGroup->instancing_[i].Get(), newGroup->instanceCount_, sizeof(TransformationParticleMatrix));
		}

		//ここでパーティクルをあらかじめ作る
		float add = 0.1f;
		for (int i = 0; i < int(max); i++) {
			Particle p{};
			p.scale = { 1.0f,1.0f,1.0f };
			p.translate.x += add;
			p.translate.y += add;
			newGroup->particles_.push_back(p);
			add += 0.1f;
		}

		parentParticleGroups_.emplace(name, std::move(newGroup));
	} else {
		auto iterator = particleGroups_.find(name);
		if (iterator != particleGroups_.end()) {
			return;
		}

		auto newGroup = std::make_unique<ParticleGroup>();
		newGroup->type_ = blendType;
		newGroup->shapeType_ = shape;
		newGroup->emitter_.name_ = name;
		newGroup->emitter_.Load(name);

		newGroup->instanceCount_ = count;
		for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
			newGroup->instancing_[i] = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(),
				sizeof(TransformationParticleMatrix) * newGroup->instanceCount_);
			newGroup->instancing_[i]->Map(0, nullptr, reinterpret_cast<void**>(&newGroup->instancingDataGPU_[i]));
			for (uint32_t index = 0; index < count; ++index) {
				newGroup->instancingDataGPU_[i][index].WVP = MakeIdentity4x4();
				newGroup->instancingDataGPU_[i][index].World = MakeIdentity4x4();
			}
		}

		newGroup->material_.SetTextureNamePath(fileName);
		newGroup->material_.CreateMaterial();
		for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
			newGroup->srvIndex_[i] = srvManager_->Allocate();
			srvManager_->CreateStructuredSRV(newGroup->srvIndex_[i], newGroup->instancing_[i].Get(),
				newGroup->instanceCount_, sizeof(TransformationParticleMatrix));
		}

		// 仮の粒子を入れる（デバッグ用途）
		float add = 0.1f;
		for (uint32_t i = 0; i < count; ++i) {
			Particle p{};
			p.scale = { 1.0f,1.0f,1.0f };
			p.translate.x += add;
			p.translate.y += add;
			newGroup->particles_.push_back(p);
			add += 0.1f;
		}

		particleGroups_.insert(std::make_pair(name, std::move(newGroup)));
	}
}

void ParticleManager::UpdateParticleGroup(const Matrix4x4& billboardMatrix) {
	for (auto& groupPair : particleGroups_) {
		ParticleGroup* group = groupPair.second.get();
#ifdef _DEBUG
		if (group->emitter_.isEmit_) {
			if (selectParticleGroup_ != group) {
				group->emitter_.Emit();
			}
		}
#endif // _DEBUG

		group->Update(billboardMatrix, camera_, dxcommon_->GetNowFrameCount());
	}
}

void ParticleManager::UpdateParentParticleGroup(const Matrix4x4& billboardMatrix) {
	for (auto& groupPair : parentParticleGroups_) {
		ParentParticleGroup* group = groupPair.second.get();
		group->Update(billboardMatrix, camera_, dxcommon_->GetNowFrameCount());
	}
}

void ParticleManager::UpdateAnimeGroup(const Matrix4x4& billboardMatrix) {
	for (auto& groupPair : animeGroups_) {
		AnimeGroup* group = groupPair.second.get();
		for (int i = 0; i < group->objects_.size(); i++) {
			if (group->lifeTime[i] <= 0) {
				group->isLive_[i] = false;
				continue;
			}

			group->lifeTime[i] -= FPSKeeper::DeltaTimeFrame();
			group->animeTime[i] += FPSKeeper::DeltaTimeFrame();

			for (auto& animeChange : group->anime_) {
				if (group->animeTime[i] >= animeChange.second * FPSKeeper::DeltaTimeFrame()) {
					group->objects_[i]->SetTexture(animeChange.first);
				}
			}

			SizeType sizeType = SizeType(group->type);
			float t = (1.0f - float(float(group->lifeTime[i]) / float(group->startLifeTime_[i])));
			switch (sizeType) {
			case SizeType::kNormal:
				break;
			case SizeType::kShift:

				group->objects_[i]->transform.scale.x = Lerp(group->startSize.x, group->endSize.x, t);
				group->objects_[i]->transform.scale.y = Lerp(group->startSize.y, group->endSize.y, t);

				break;
			}

			group->speed[i] += group->accele[i] * FPSKeeper::DeltaTimeFrame();

			group->objects_[i]->transform.translate += group->speed[i] * FPSKeeper::DeltaTimeFrame();
			group->objects_[i]->SetBillboardMat(billboardMatrix);
		}
	}
}

void ParticleManager::DrawParticleGroup() {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	for (auto& groupPair : particleGroups_) {
		ParticleGroup* group = groupPair.second.get();
		if (group->drawCount_ == 0) continue;
		if (group->shapeType_ == ShapeType::LIGHTNING) continue;
		if (preType_ != group->type_) {
			switch (group->type_) {
			case BlendType::ALPHA:
				dxcommon_->GetPipelineManager()->SetPipeline(Pipe::ParticleAlpha);
				break;
			case BlendType::ADD:
				dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Particle);
				break;
			case BlendType::SUBTRACT:
				dxcommon_->GetPipelineManager()->SetPipeline(Pipe::ParticleSub);
				break;
			case BlendType::SCREEN:
				dxcommon_->GetPipelineManager()->SetPipeline(Pipe::ParticleScreen);
				break;
			case BlendType::MULTIPLY:
				dxcommon_->GetPipelineManager()->SetPipeline(Pipe::ParticleMultiply);
				break;
			case BlendType::SOFT_ADD:
				dxcommon_->GetPipelineManager()->SetPipeline(Pipe::ParticleSoftAdd);
				break;
			case BlendType::PREMULTIPLIED_ALPHA:
				dxcommon_->GetPipelineManager()->SetPipeline(Pipe::ParticlePreMulAlpha);
				break;
			default:
				dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Particle);
				break;
			}
		}

		ShapeTypeCommand(group->shapeType_);
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(1, srvManager_->GetGPUDescriptorHandle(group->srvIndex_[frameIndex]));
		dxcommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, group->material_.GetMaterialResource()->GetGPUVirtualAddress());
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, group->material_.GetTexture()->gpuHandle);
		ShapeTypeDrawCommand(group->shapeType_, group->drawCount_);

		preType_ = group->type_;
	}


	for (auto& groupPair : particleGroups_) {
		ParticleGroup* group = groupPair.second.get();
		if (group->drawCount_ == 0) continue;
		if (group->shapeType_ != ShapeType::LIGHTNING) continue;
		if (preType_ != group->type_) {
			switch (group->type_) {
			case BlendType::ALPHA:
				dxcommon_->GetPipelineManager()->SetPipeline(Pipe::ParticleAlpha);
				break;
			case BlendType::ADD:
				dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Particle);
				break;
			case BlendType::SUBTRACT:
				dxcommon_->GetPipelineManager()->SetPipeline(Pipe::ParticleSub);
				break;
			case BlendType::SCREEN:
				dxcommon_->GetPipelineManager()->SetPipeline(Pipe::ParticleScreen);
				break;
			case BlendType::MULTIPLY:
				dxcommon_->GetPipelineManager()->SetPipeline(Pipe::ParticleMultiply);
				break;
			case BlendType::SOFT_ADD:
				dxcommon_->GetPipelineManager()->SetPipeline(Pipe::ParticleSoftAdd);
				break;
			case BlendType::PREMULTIPLIED_ALPHA:
				dxcommon_->GetPipelineManager()->SetPipeline(Pipe::ParticlePreMulAlpha);
				break;
			default:
				break;
			}
		}

		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(1, srvManager_->GetGPUDescriptorHandle(group->srvIndex_[frameIndex]));
		lightning_->MeshDraw(&group->material_, group->drawCount_);

		if (group->shapeType_ != ShapeType::PLANE) {
			dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &plane_.vbView);
			dxcommon_->GetCommandList()->IASetIndexBuffer(&plane_.ibView);
		}

		preType_ = group->type_;
	}
}

void ParticleManager::DrawParentParticleGroup() {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	for (auto& groupPair : parentParticleGroups_) {
		ParentParticleGroup* group = groupPair.second.get();
		if (group->drawCount_ == 0) continue;
		if (group->shapeType_ == ShapeType::LIGHTNING) continue;

		ShapeTypeCommand(group->shapeType_);
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(1, srvManager_->GetGPUDescriptorHandle(group->srvIndex_[frameIndex]));
		dxcommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, group->material_.GetMaterialResource()->GetGPUVirtualAddress());
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, group->material_.GetTexture()->gpuHandle);
		ShapeTypeDrawCommand(group->shapeType_, group->drawCount_);
	}
	for (auto& groupPair : parentParticleGroups_) {
		ParentParticleGroup* group = groupPair.second.get();
		if (group->drawCount_ == 0) continue;
		if (group->shapeType_ != ShapeType::LIGHTNING) continue;

		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(1, srvManager_->GetGPUDescriptorHandle(group->srvIndex_[frameIndex]));
		lightning_->MeshDraw(&group->material_, group->drawCount_);

		if (group->shapeType_ != ShapeType::PLANE) {
			dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &plane_.vbView);
			dxcommon_->GetCommandList()->IASetIndexBuffer(&plane_.ibView);
		}
	}
}

void ParticleManager::ShapeTypeCommand(const ShapeType& type) {
	if (type != ShapeType::PLANE) {
		switch (type) {
		case ShapeType::PLANE:
			break;
		case ShapeType::RING:
			dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &ring_.vbView);
			dxcommon_->GetCommandList()->IASetIndexBuffer(&ring_.ibView);
			break;
		case ShapeType::SPHERE:
			dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &sphere_.vbView);
			dxcommon_->GetCommandList()->IASetIndexBuffer(&sphere_.ibView);
			break;
		case ShapeType::TORUS:
			break;
		case ShapeType::CYLINDER:
			dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &cylinder_.vbView);
			dxcommon_->GetCommandList()->IASetIndexBuffer(&cylinder_.ibView);
			break;
		case ShapeType::CONE:
			break;
		case ShapeType::TRIANGLE:
			break;
		case ShapeType::BOX:
			break;
		default:
			break;
		}
	}
}

void ParticleManager::ShapeTypeDrawCommand(const ShapeType& type, uint32_t count) {
	switch (type) {
	case ShapeType::PLANE:
		dxcommon_->GetCommandList()->DrawIndexedInstanced(static_cast<UINT>((plane_.indices.size())), count, 0, 0, 0);
		break;
	case ShapeType::RING:
		dxcommon_->GetCommandList()->DrawIndexedInstanced(static_cast<UINT>((ring_.indices.size())), count, 0, 0, 0);
		break;
	case ShapeType::SPHERE:
		dxcommon_->GetCommandList()->DrawIndexedInstanced(static_cast<UINT>((sphere_.indices.size())), count, 0, 0, 0);
		break;
	case ShapeType::TORUS:
		break;
	case ShapeType::CYLINDER:
		dxcommon_->GetCommandList()->DrawIndexedInstanced(static_cast<UINT>((cylinder_.indices.size())), count, 0, 0, 0);
		break;
	case ShapeType::CONE:
		break;
	case ShapeType::TRIANGLE:
		break;
	case ShapeType::BOX:
		break;
	default:
		break;
	}

	if (type != ShapeType::PLANE) {
		dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &plane_.vbView);
		dxcommon_->GetCommandList()->IASetIndexBuffer(&plane_.ibView);
	}
}

void ParticleManager::InitPlaneVertex() {
	plane_.vertices.push_back({ {-1.0f,1.0f,0.0f,1.0f},{0.0f,0.0f},{0.0f,0.0f,-1.0f} });
	plane_.vertices.push_back({ {-1.0f,-1.0f,0.0f,1.0f},{0.0f,1.0f},{0.0f,0.0f,-1.0f} });
	plane_.vertices.push_back({ {1.0f,-1.0f,0.0f,1.0f},{1.0f,1.0f},{0.0f,0.0f,-1.0f} });
	plane_.vertices.push_back({ {1.0f,1.0f,0.0f,1.0f},{1.0f,0.0f},{0.0f,0.0f,-1.0f} });

	plane_.indices.push_back(0);
	plane_.indices.push_back(3);
	plane_.indices.push_back(1);

	plane_.indices.push_back(1);
	plane_.indices.push_back(3);
	plane_.indices.push_back(2);

	plane_.vBuffer = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(VertexDate) * plane_.vertices.size());
	plane_.iBuffer = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(uint32_t) * plane_.indices.size());

	VertexDate* vData = nullptr;
	plane_.vBuffer->Map(0, nullptr, reinterpret_cast<void**>(&vData));
	std::memcpy(vData, plane_.vertices.data(), sizeof(VertexDate) * plane_.vertices.size());

	plane_.vbView.BufferLocation = plane_.vBuffer->GetGPUVirtualAddress();
	plane_.vbView.SizeInBytes = static_cast<UINT>(sizeof(VertexDate) * plane_.vertices.size());
	plane_.vbView.StrideInBytes = static_cast<UINT>(sizeof(VertexDate));

	uint32_t* iData = nullptr;
	plane_.iBuffer->Map(0, nullptr, reinterpret_cast<void**>(&iData));
	std::memcpy(iData, plane_.indices.data(), sizeof(uint32_t) * plane_.indices.size());

	plane_.ibView.BufferLocation = plane_.iBuffer->GetGPUVirtualAddress();
	plane_.ibView.Format = DXGI_FORMAT_R32_UINT;
	plane_.ibView.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * plane_.indices.size());
}

void ParticleManager::InitRingVertex() {
	const uint32_t kRingDivide = 32;
	const float kOuterRadius = 1.0f;
	const float kInnerRadius = 0.2f;
	const float radianPerDivide = 2.0f * std::numbers::pi_v<float> / float(kRingDivide);

	for (uint32_t i = 0; i <= kRingDivide; i++) {
		float angle = i * radianPerDivide;
		float sinA = std::sin(angle);
		float cosA = std::cos(angle);
		float u = float(i) / float(kRingDivide);

		// 外周
		ring_.vertices.push_back({ {-sinA * kOuterRadius, cosA * kOuterRadius, 0.0f, 1.0f}, {u, 0.0f}, {0,0,1} });
		// 内周
		ring_.vertices.push_back({ {-sinA * kInnerRadius, cosA * kInnerRadius, 0.0f, 1.0f}, {u, 1.0f}, {0,0,1} });
	}

	// インデックス生成
	for (uint32_t i = 0; i < kRingDivide; i++) {
		uint32_t outer0 = i * 2;
		uint32_t inner0 = outer0 + 1;
		uint32_t outer1 = outer0 + 2;
		uint32_t inner1 = outer0 + 3;

		// 三角形1
		ring_.indices.push_back(outer0);
		ring_.indices.push_back(inner0);
		ring_.indices.push_back(outer1);

		// 三角形2
		ring_.indices.push_back(outer1);
		ring_.indices.push_back(inner0);
		ring_.indices.push_back(inner1);
	}

	ring_.vBuffer = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(VertexDate) * ring_.vertices.size());
	ring_.iBuffer = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(uint32_t) * ring_.indices.size());

	VertexDate* vData = nullptr;
	ring_.vBuffer->Map(0, nullptr, reinterpret_cast<void**>(&vData));
	std::memcpy(vData, ring_.vertices.data(), sizeof(VertexDate) * ring_.vertices.size());

	ring_.vbView.BufferLocation = ring_.vBuffer->GetGPUVirtualAddress();
	ring_.vbView.SizeInBytes = static_cast<UINT>(sizeof(VertexDate) * ring_.vertices.size());
	ring_.vbView.StrideInBytes = static_cast<UINT>(sizeof(VertexDate));

	uint32_t* iData = nullptr;
	ring_.iBuffer->Map(0, nullptr, reinterpret_cast<void**>(&iData));
	std::memcpy(iData, ring_.indices.data(), sizeof(uint32_t) * ring_.indices.size());

	ring_.ibView.BufferLocation = ring_.iBuffer->GetGPUVirtualAddress();
	ring_.ibView.Format = DXGI_FORMAT_R32_UINT;
	ring_.ibView.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * ring_.indices.size());
}

void ParticleManager::InitSphereVertex() {
	const float pi = 3.1415926535f;
	const uint32_t kSubdivision = 16;

	const float kLonEvery = (2.0f * pi) / static_cast<float>(kSubdivision);
	const float kLatEvery = pi / static_cast<float>(kSubdivision);

	// 頂点生成
	for (uint32_t latIndex = 0; latIndex <= kSubdivision; ++latIndex) {
		float lat = -pi / 2.0f + kLatEvery * latIndex;
		float v = 1.0f - float(latIndex) / float(kSubdivision); // 上がv=0, 下がv=1になるように

		for (uint32_t lonIndex = 0; lonIndex <= kSubdivision; ++lonIndex) {
			// 経度ループ用に +1 まで回す
			float lon = lonIndex * kLonEvery;
			float u = float(lonIndex) / float(kSubdivision); // 経度でuを算出（0〜1）

			float x = cosf(lat) * cosf(lon);
			float y = sinf(lat);
			float z = cosf(lat) * sinf(lon);

			sphere_.vertices.push_back({ {x, y, z, 1.0f},{u, v},{x, y, z} });
		}
	}

	// インデックス生成
	for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
			uint32_t row1 = latIndex * (kSubdivision + 1);
			uint32_t row2 = (latIndex + 1) * (kSubdivision + 1);

			uint32_t v0 = row1 + lonIndex;
			uint32_t v1 = row1 + lonIndex + 1;
			uint32_t v2 = row2 + lonIndex;
			uint32_t v3 = row2 + lonIndex + 1;

			sphere_.indices.push_back(v0);
			sphere_.indices.push_back(v2);
			sphere_.indices.push_back(v1);

			sphere_.indices.push_back(v1);
			sphere_.indices.push_back(v2);
			sphere_.indices.push_back(v3);
		}
	}

	sphere_.vBuffer = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(VertexDate) * sphere_.vertices.size());
	sphere_.iBuffer = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(uint32_t) * sphere_.indices.size());

	VertexDate* vData = nullptr;
	sphere_.vBuffer->Map(0, nullptr, reinterpret_cast<void**>(&vData));
	std::memcpy(vData, sphere_.vertices.data(), sizeof(VertexDate) * sphere_.vertices.size());

	sphere_.vbView.BufferLocation = sphere_.vBuffer->GetGPUVirtualAddress();
	sphere_.vbView.SizeInBytes = static_cast<UINT>(sizeof(VertexDate) * sphere_.vertices.size());
	sphere_.vbView.StrideInBytes = static_cast<UINT>(sizeof(VertexDate));

	uint32_t* iData = nullptr;
	sphere_.iBuffer->Map(0, nullptr, reinterpret_cast<void**>(&iData));
	std::memcpy(iData, sphere_.indices.data(), sizeof(uint32_t) * sphere_.indices.size());

	sphere_.ibView.BufferLocation = sphere_.iBuffer->GetGPUVirtualAddress();
	sphere_.ibView.Format = DXGI_FORMAT_R32_UINT;
	sphere_.ibView.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * sphere_.indices.size());
}

void ParticleManager::InitCylinderVertex() {
	const uint32_t kCylinderDivide = 32;
	const float kTopRadius = 1.0f;
	const float kBottomRadius = 1.0f;
	const float kHeight = 2.0f;
	const float radianPerDivide = 2.0f * std::numbers::pi_v<float> / float(kCylinderDivide);

	for (uint32_t i = 0; i <= kCylinderDivide; i++) {
		float angle = i * radianPerDivide;
		float sinA = std::sin(angle);
		float cosA = std::cos(angle);
		float u = float(i) / float(kCylinderDivide);

		// 下
		Vector3 posBottom = { cosA * kBottomRadius, 0.0f, sinA * kBottomRadius };
		Vector3 normal = { cosA, 0.0f, sinA };
		cylinder_.vertices.push_back({ {posBottom.x, posBottom.y, posBottom.z, 1.0f}, {u, 1.0f}, normal });

		// 上
		Vector3 posTop = { cosA * kTopRadius, kHeight, sinA * kTopRadius };
		cylinder_.vertices.push_back({ {posTop.x, posTop.y, posTop.z, 1.0f}, {u, 0.0f}, normal });
	}

	// インデックス生成
	for (uint32_t i = 0; i < kCylinderDivide; i++) {
		uint32_t bottom0 = i * 2;
		uint32_t top0 = bottom0 + 1;
		uint32_t bottom1 = bottom0 + 2;
		uint32_t top1 = bottom0 + 3;

		// 三角形1
		cylinder_.indices.push_back(bottom0);
		cylinder_.indices.push_back(top0);
		cylinder_.indices.push_back(bottom1);

		// 三角形2
		cylinder_.indices.push_back(bottom1);
		cylinder_.indices.push_back(top0);
		cylinder_.indices.push_back(top1);
	}

	cylinder_.vBuffer = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(VertexDate) * cylinder_.vertices.size());
	cylinder_.iBuffer = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(uint32_t) * cylinder_.indices.size());

	VertexDate* vData = nullptr;
	cylinder_.vBuffer->Map(0, nullptr, reinterpret_cast<void**>(&vData));
	std::memcpy(vData, cylinder_.vertices.data(), sizeof(VertexDate) * cylinder_.vertices.size());

	cylinder_.vbView.BufferLocation = cylinder_.vBuffer->GetGPUVirtualAddress();
	cylinder_.vbView.SizeInBytes = static_cast<UINT>(sizeof(VertexDate) * cylinder_.vertices.size());
	cylinder_.vbView.StrideInBytes = static_cast<UINT>(sizeof(VertexDate));

	uint32_t* iData = nullptr;
	cylinder_.iBuffer->Map(0, nullptr, reinterpret_cast<void**>(&iData));
	std::memcpy(iData, cylinder_.indices.data(), sizeof(uint32_t) * cylinder_.indices.size());

	cylinder_.ibView.BufferLocation = cylinder_.iBuffer->GetGPUVirtualAddress();
	cylinder_.ibView.Format = DXGI_FORMAT_R32_UINT;
	cylinder_.ibView.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * cylinder_.indices.size());
}

void ParticleManager::InitLighningVertex() {
	lightning_ = std::make_unique<Object3d>();
	lightning_->Create("lightning.obj");
}

void ParticleManager::ParticleTexurePopUp() {
#ifdef _DEBUG
	if (ImGui::Button("TextureFile")) {
		ImGui::OpenPopup("TextureFile Window");
	}
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0, 0, 0, 0.25f));
	if (ImGui::BeginPopupModal("TextureFile Window", NULL)) {
		if (ImGui::Button("Refresh")) {
			TextureManager::GetInstance()->LoadTextureFile(true);
		}
		int buttonCount = 0;
		for (const auto& TexName : TextureManager::GetInstance()->GetTextureFiles()) {
			if (buttonCount > 0 && buttonCount < 5) {
				ImGui::SameLine();
			} else {
				buttonCount = 0;
			}
			if (ImGui::ImageButton(("##"+ TexName.first).c_str(), (ImTextureID)TextureManager::GetInstance()->GetTexture(TexName.first.c_str())->gpuHandle.ptr, ImVec2(100, 100))) {
				selectParticleGroup_->material_.SetTextureNamePath(TexName.first.c_str());
				selectParticleGroup_->material_.SetTexture(TexName.first.c_str(), TexName.second);
				if (TexName.second) {
					TextureManager::GetInstance()->SetTextureFileOnceLoad(TexName.first.c_str());
				}
			}
			buttonCount++;
		}
		ImGui::Separator();
		if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::EndPopup();
	}
	ImGui::PopStyleColor();
#endif // _DEBUG
}

void ParticleManager::SaveGroupData() {
#ifdef _DEBUG
	json data{};
	data["name"] = currentKey_;
	data["texName"] = selectParticleGroup_->material_.GetPathName();
	data["count"] = selectParticleGroup_->instanceCount_;
	data["Shape"] = static_cast<int>(selectParticleGroup_->shapeType_);
	data["subMode"] = selectParticleGroup_->isSubMode_;
	JsonSerializer::SerializeJsonData(data, ("resource/ParticleGroups/" + currentKey_ + ".json").c_str());
#endif // _DEBUG
}

void ParticleManager::JsonCheckForGroup(const std::string& name, const std::string& fileName, uint32_t count, ShapeType shape, BlendType blendType, bool isParnt) {
	std::string path = "resource/ParticleGroups/" + name + ".json";

	// JSON があるかチェック
	if (std::filesystem::exists(path)) {
		// JSON を読み込んでパラメータを設定
		json data = JsonSerializer::DeserializeJsonData(path.c_str());

		// パラメータ取得
		std::string texName = data["texName"];
		uint32_t maxCount = data["count"];
		ShapeType shapeType = static_cast<ShapeType>(data["Shape"].get<int>());
		BlendType type = static_cast<BlendType>(data.value("blendType", 1));

		InternalCreateParticleGroup(name, texName, maxCount, shapeType, type, isParnt);
	} else {
		InternalCreateParticleGroup(name, fileName, count, shape, blendType, isParnt);
		
		json data{};
		data["name"] = name;
		data["texName"] = fileName;
		data["count"] = count;
		data["Shape"] = static_cast<int>(shape);
		data["blendType"] = blendType;
		JsonSerializer::SerializeJsonData(data, path.c_str());
	}
}
