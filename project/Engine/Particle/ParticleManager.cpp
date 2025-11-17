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

	vBuffer_.Reset();
	iBuffer_.Reset();
	ringVBuffer_.Reset();
	ringIBuffer_.Reset();
	sphereVBuffer_.Reset();
	sphereIBuffer_.Reset();
	cylinderIBuffer_.Reset();
	cylinderVBuffer_.Reset();
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
	gpuParticleSystem_->Draw(vbView, ibView);

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
	dxcommon_->GetPipelineManager()->SetPipeline(Pipe::particle);
	dxcommon_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vbView);
	dxcommon_->GetCommandList()->IASetIndexBuffer(&ibView);
	DrawParticleGroup();
	DrawParentParticleGroup();

	dxcommon_->GetDXCommand()->SetViewAndscissor();
	dxcommon_->GetPipelineManager()->SetPipeline(Pipe::particleSub);
	dxcommon_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vbView);
	dxcommon_->GetCommandList()->IASetIndexBuffer(&ibView);
	for (auto& groupPair : particleGroups_) {
		ParticleGroup* group = groupPair.second.get();
		if (!group->isSubMode_) continue;
		if (group->drawCount_ == 0) continue;

		ShapeTypeCommand(group->shapeType_);
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(1, srvManager_->GetGPUDescriptorHandle(group->srvIndex_));
		dxcommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, group->material_.GetMaterialResource()->GetGPUVirtualAddress());
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, group->material_.GetTexture()->gpuHandle);
		ShapeTypeDrawCommand(group->shapeType_, group->drawCount_);
	}
}

void ParticleManager::ParticleDebugGUI() {
#ifdef _DEBUG
	ImGui::Begin("ParticleDebug");
	ImGui::SeparatorText("ParticleGroup");
	if (particleGroups_.size() != 0) {
		if (!selectParticleGroup_) {
			selectParticleGroup_ = particleGroups_.begin()->second.get();
		}
	}

	std::vector<const char*> keys;
	for (const auto& pair : particleGroups_) {
		keys.push_back(pair.first.c_str());
	}
	if (ImGui::Combo("Particle Group", &currentIndex_, keys.data(), int(keys.size()))) {
		currentKey_ = keys[currentIndex_];
		// currentKey を使って選択中の ParticleGroup を取得
		selectParticleGroup_ = particleGroups_[currentKey_].get();
	}
	if (selectParticleGroup_) {
		int shapeType = static_cast<int>(selectParticleGroup_->shapeType_);
		ImGui::Combo("ShapeType##type", &shapeType, "Plane\0Ring\0sphere\0Torus\0Cylinder\0Cone\0Triangle\0Box\0Lightning\0");
		selectParticleGroup_->shapeType_ = static_cast<ShapeType>(shapeType);
		ImGui::Image((ImTextureID)TextureManager::GetInstance()->GetTexture(selectParticleGroup_->material_.GetPathName().c_str())->gpuHandle.ptr, { 100,100 });
		ParticleTexurePopUp();
		int maxCount = static_cast<uint32_t>(selectParticleGroup_->insstanceCount_);
		ImGui::DragInt("maxCount", &maxCount, 1, 1, 3000);
		selectParticleGroup_->insstanceCount_ = static_cast<uint32_t>(maxCount);
		ImGui::Text("count : %d", int(selectParticleGroup_->drawCount_));
		if (ImGui::Button("SaveGroup")) {
			SaveGroupData();
		}
	}

	ImGui::SeparatorText("SelectGroup");
	if (selectParticleGroup_) {
		ParticleEmitter& selecrtEmitter = selectParticleGroup_->emitter_;
		selecrtEmitter.DebugGUI();
	}

	ImGui::SeparatorText("Emit Control");
	if (ImGui::TreeNode("ParticleGroup Emit Control")) {
		static ParticleGroupSelector selector;
		if (ImGui::Button("ResetFrenquencyTime")) {
			for (auto& groupPair : particleGroups_) {
				groupPair.second->emitter_.TimeReset();
			}
		}

		// 初期化（1回だけ）
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

void ParticleManager::CreateParticleGroup(const std::string& name, const std::string& fileName, uint32_t count, ShapeType shape, bool subMode) {
	ParticleManager* instance = GetInstance();
	auto iterator = instance->particleGroups_.find(name);
	if (iterator != instance->particleGroups_.end()) {
		return;
	}

	std::string path = "resource/ParticleGroups/" + name + ".json";

	// JSON があるかチェック
	if (std::filesystem::exists(path)) {
		// JSON を読み込んでパラメータを設定
		json data = JsonSerializer::DeserializeJsonData(path.c_str());

		// パラメータ取得
		std::string texName = data["texName"];
		uint32_t maxCount = data["count"];
		ShapeType shapeType = static_cast<ShapeType>(data["Shape"].get<int>());
		bool isSubMode = data["subMode"];

		instance->InternalCreateParticleGroup(name, texName, maxCount, shapeType, isSubMode);
	} else {
		instance->InternalCreateParticleGroup(name, fileName, count, shape, subMode);

		json data{};
		data["name"] = name;
		data["texName"] = fileName;
		data["count"] = count;
		data["Shape"] = static_cast<int>(shape);
		data["subMode"] = subMode;
		JsonSerializer::SerializeJsonData(data, path.c_str());
	}
}

void ParticleManager::CreateParentParticleGroup(const std::string& name, const std::string& fileName, uint32_t count, ShapeType shape) {
	ParticleManager* instance = GetInstance();
	auto iterator = instance->parentParticleGroups_.find(name);
	if (iterator != instance->parentParticleGroups_.end()) {
		return;
	}

	auto newGroup = std::make_unique<ParentParticleGroup>();
	newGroup->emitter_ = std::make_unique<ParticleEmitter>();
	newGroup->shapeType_ = shape;
	newGroup->emitter_->name_ = name;
	newGroup->emitter_->Load(name);

	newGroup->insstanceCount_ = count;
	newGroup->instancing_ = instance->dxcommon_->CreateBufferResource(instance->dxcommon_->GetDevice(), (sizeof(TransformationParticleMatrix) * newGroup->insstanceCount_));
	newGroup->instancing_->Map(0, nullptr, reinterpret_cast<void**>(&newGroup->instancingData_));
	uint32_t max = newGroup->insstanceCount_;
	for (uint32_t index = 0; index < max; ++index) {
		newGroup->instancingData_[index].WVP = MakeIdentity4x4();
		newGroup->instancingData_[index].World = MakeIdentity4x4();
	}
	newGroup->material_.SetTextureNamePath(fileName);
	newGroup->material_.CreateMaterial();
	newGroup->srvIndex_ = instance->srvManager_->Allocate();
	instance->srvManager_->CreateStructuredSRV(newGroup->srvIndex_, newGroup->instancing_.Get(), newGroup->insstanceCount_, sizeof(TransformationParticleMatrix));

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

	instance->parentParticleGroups_.emplace(name, std::move(newGroup));
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

	} else {
		return;
	}
}

void ParticleManager::Emit(const std::string& name, const Vector3& pos, const Vector3& rotate, const Particle& grain, const RandomParametor& para, uint32_t count) {
	ParticleManager* instance = GetInstance();

	auto iterator = instance->particleGroups_.find(name);
	if (iterator != instance->particleGroups_.end()) {
		uint32_t newCount = 0;

		ParticleGroup* group = iterator->second.get();
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

void ParticleManager::ParentEmit(const std::string& name, const Vector3& pos, const Vector3& rotate, const Particle& grain, const RandomParametor& para, uint32_t count) {
	ParticleManager* instance = GetInstance();

	auto iterator = instance->parentParticleGroups_.find(name);
	if (iterator != instance->parentParticleGroups_.end()) {
		uint32_t newCount = 0;

		ParentParticleGroup* group = iterator->second.get();
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

int ParticleManager::InitGPUEmitter() {
	return gpuParticleSystem_->InitGPUEmitter();
}

int ParticleManager::InitGPUEmitterTexture(const std::string& fileName) {
	return gpuParticleSystem_->InitGPUEmitterTexture(fileName);
}

int ParticleManager::InitGPUEmitterSurface(const std::string& fileName) {
	return gpuParticleSystem_->InitGPUEmitterSurface(fileName);
}

void ParticleManager::InternalCreateParticleGroup(const std::string& name, const std::string& fileName, uint32_t count, ShapeType shape, bool subMode) {
	auto iterator = particleGroups_.find(name);
	if (iterator != particleGroups_.end()) {
		return;
	}

	ParticleGroup* newGroup = new ParticleGroup();
	newGroup->isSubMode_ = subMode;
	newGroup->shapeType_ = shape;
	newGroup->emitter_.name_ = name;
	newGroup->emitter_.Load(name);

	newGroup->insstanceCount_ = count;
	newGroup->instancing_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(),
		sizeof(TransformationParticleMatrix) * newGroup->insstanceCount_);
	newGroup->instancing_->Map(0, nullptr, reinterpret_cast<void**>(&newGroup->instancingData_));

	for (uint32_t index = 0; index < count; ++index) {
		newGroup->instancingData_[index].WVP = MakeIdentity4x4();
		newGroup->instancingData_[index].World = MakeIdentity4x4();
	}

	newGroup->material_.SetTextureNamePath(fileName);
	newGroup->material_.CreateMaterial();
	newGroup->srvIndex_ = srvManager_->Allocate();
	srvManager_->CreateStructuredSRV(newGroup->srvIndex_, newGroup->instancing_.Get(),
		newGroup->insstanceCount_, sizeof(TransformationParticleMatrix));

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

	particleGroups_.insert(std::make_pair(name, newGroup));
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

		group->Update(billboardMatrix, camera_);
	}
}

void ParticleManager::UpdateParentParticleGroup(const Matrix4x4& billboardMatrix) {
	for (auto& groupPair : parentParticleGroups_) {
		ParentParticleGroup* group = groupPair.second.get();
		group->Update(billboardMatrix, camera_);
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

			group->lifeTime[i] -= FPSKeeper::DeltaTime();
			group->animeTime[i] += FPSKeeper::DeltaTime();

			for (auto& animeChange : group->anime_) {
				if (group->animeTime[i] >= animeChange.second * FPSKeeper::DeltaTime()) {
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

			group->speed[i] += group->accele[i] * FPSKeeper::DeltaTime();

			group->objects_[i]->transform.translate += group->speed[i] * FPSKeeper::DeltaTime();
			group->objects_[i]->SetBillboardMat(billboardMatrix);
		}
	}
}

void ParticleManager::DrawParticleGroup() {
	for (auto& groupPair : particleGroups_) {
		ParticleGroup* group = groupPair.second.get();
		if (group->isSubMode_) continue;
		if (group->drawCount_ == 0) continue;
		if (group->shapeType_ == ShapeType::LIGHTNING) continue;

		ShapeTypeCommand(group->shapeType_);
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(1, srvManager_->GetGPUDescriptorHandle(group->srvIndex_));
		dxcommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, group->material_.GetMaterialResource()->GetGPUVirtualAddress());
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, group->material_.GetTexture()->gpuHandle);
		ShapeTypeDrawCommand(group->shapeType_, group->drawCount_);
	}
	for (auto& groupPair : particleGroups_) {
		ParticleGroup* group = groupPair.second.get();
		if (group->isSubMode_) continue;
		if (group->drawCount_ == 0) continue;
		if (group->shapeType_ != ShapeType::LIGHTNING) continue;

		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(1, srvManager_->GetGPUDescriptorHandle(group->srvIndex_));
		lightning_->MeshDraw(&group->material_, group->drawCount_);

		if (group->shapeType_ != ShapeType::PLANE) {
			dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vbView);
			dxcommon_->GetCommandList()->IASetIndexBuffer(&ibView);
		}
	}
}

void ParticleManager::DrawParentParticleGroup() {
	for (auto& groupPair : parentParticleGroups_) {
		ParentParticleGroup* group = groupPair.second.get();
		if (group->drawCount_ == 0) continue;
		if (group->shapeType_ == ShapeType::LIGHTNING) continue;

		ShapeTypeCommand(group->shapeType_);
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(1, srvManager_->GetGPUDescriptorHandle(group->srvIndex_));
		dxcommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, group->material_.GetMaterialResource()->GetGPUVirtualAddress());
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, group->material_.GetTexture()->gpuHandle);
		ShapeTypeDrawCommand(group->shapeType_, group->drawCount_);
	}
	for (auto& groupPair : parentParticleGroups_) {
		ParentParticleGroup* group = groupPair.second.get();
		if (group->drawCount_ == 0) continue;
		if (group->shapeType_ != ShapeType::LIGHTNING) continue;

		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(1, srvManager_->GetGPUDescriptorHandle(group->srvIndex_));
		lightning_->MeshDraw(&group->material_, group->drawCount_);

		if (group->shapeType_ != ShapeType::PLANE) {
			dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vbView);
			dxcommon_->GetCommandList()->IASetIndexBuffer(&ibView);
		}
	}
}

void ParticleManager::ShapeTypeCommand(const ShapeType& type) {
	if (type != ShapeType::PLANE) {
		switch (type) {
		case ShapeType::PLANE:
			break;
		case ShapeType::RING:
			dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &ringVbView);
			dxcommon_->GetCommandList()->IASetIndexBuffer(&ringIbView);
			break;
		case ShapeType::SPHERE:
			dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &sphereVbView);
			dxcommon_->GetCommandList()->IASetIndexBuffer(&sphereIbView);
			break;
		case ShapeType::TORUS:
			break;
		case ShapeType::CYLINDER:
			dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &cylinderVbView);
			dxcommon_->GetCommandList()->IASetIndexBuffer(&cylinderIbView);
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
		dxcommon_->GetCommandList()->DrawIndexedInstanced(static_cast<UINT>((index_.size())), count, 0, 0, 0);
		break;
	case ShapeType::RING:
		dxcommon_->GetCommandList()->DrawIndexedInstanced(static_cast<UINT>((ringIndex_.size())), count, 0, 0, 0);
		break;
	case ShapeType::SPHERE:
		dxcommon_->GetCommandList()->DrawIndexedInstanced(static_cast<UINT>((sphereIndex_.size())), count, 0, 0, 0);
		break;
	case ShapeType::TORUS:
		break;
	case ShapeType::CYLINDER:
		dxcommon_->GetCommandList()->DrawIndexedInstanced(static_cast<UINT>((cylinderIndex_.size())), count, 0, 0, 0);
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
		dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vbView);
		dxcommon_->GetCommandList()->IASetIndexBuffer(&ibView);
	}
}

void ParticleManager::InitPlaneVertex() {
	vertex_.push_back({ {-1.0f,1.0f,0.0f,1.0f},{0.0f,0.0f},{0.0f,0.0f,-1.0f} });
	vertex_.push_back({ {-1.0f,-1.0f,0.0f,1.0f},{0.0f,1.0f},{0.0f,0.0f,-1.0f} });
	vertex_.push_back({ {1.0f,-1.0f,0.0f,1.0f},{1.0f,1.0f},{0.0f,0.0f,-1.0f} });
	vertex_.push_back({ {1.0f,1.0f,0.0f,1.0f},{1.0f,0.0f},{0.0f,0.0f,-1.0f} });

	index_.push_back(0);
	index_.push_back(3);
	index_.push_back(1);

	index_.push_back(1);
	index_.push_back(3);
	index_.push_back(2);

	vBuffer_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(VertexDate) * vertex_.size());
	iBuffer_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(uint32_t) * index_.size());

	VertexDate* vData = nullptr;
	vBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&vData));
	std::memcpy(vData, vertex_.data(), sizeof(VertexDate) * vertex_.size());

	vbView.BufferLocation = vBuffer_->GetGPUVirtualAddress();
	vbView.SizeInBytes = static_cast<UINT>(sizeof(VertexDate) * vertex_.size());
	vbView.StrideInBytes = static_cast<UINT>(sizeof(VertexDate));

	uint32_t* iData = nullptr;
	iBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&iData));
	std::memcpy(iData, index_.data(), sizeof(uint32_t) * index_.size());

	ibView.BufferLocation = iBuffer_->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R32_UINT;
	ibView.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * index_.size());
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
		ringVertex_.push_back({ {-sinA * kOuterRadius, cosA * kOuterRadius, 0.0f, 1.0f}, {u, 0.0f}, {0,0,1} });
		// 内周
		ringVertex_.push_back({ {-sinA * kInnerRadius, cosA * kInnerRadius, 0.0f, 1.0f}, {u, 1.0f}, {0,0,1} });
	}

	// インデックス生成
	for (uint32_t i = 0; i < kRingDivide; i++) {
		uint32_t outer0 = i * 2;
		uint32_t inner0 = outer0 + 1;
		uint32_t outer1 = outer0 + 2;
		uint32_t inner1 = outer0 + 3;

		// 三角形1
		ringIndex_.push_back(outer0);
		ringIndex_.push_back(inner0);
		ringIndex_.push_back(outer1);

		// 三角形2
		ringIndex_.push_back(outer1);
		ringIndex_.push_back(inner0);
		ringIndex_.push_back(inner1);
	}

	ringVBuffer_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(VertexDate) * ringVertex_.size());
	ringIBuffer_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(uint32_t) * ringIndex_.size());

	VertexDate* vData = nullptr;
	ringVBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&vData));
	std::memcpy(vData, ringVertex_.data(), sizeof(VertexDate) * ringVertex_.size());

	ringVbView.BufferLocation = ringVBuffer_->GetGPUVirtualAddress();
	ringVbView.SizeInBytes = static_cast<UINT>(sizeof(VertexDate) * ringVertex_.size());
	ringVbView.StrideInBytes = static_cast<UINT>(sizeof(VertexDate));

	uint32_t* iData = nullptr;
	ringIBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&iData));
	std::memcpy(iData, ringIndex_.data(), sizeof(uint32_t) * ringIndex_.size());

	ringIbView.BufferLocation = ringIBuffer_->GetGPUVirtualAddress();
	ringIbView.Format = DXGI_FORMAT_R32_UINT;
	ringIbView.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * ringIndex_.size());
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

			sphereVertex_.push_back({ {x, y, z, 1.0f},{u, v},{x, y, z} });
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

			sphereIndex_.push_back(v0);
			sphereIndex_.push_back(v2);
			sphereIndex_.push_back(v1);

			sphereIndex_.push_back(v1);
			sphereIndex_.push_back(v2);
			sphereIndex_.push_back(v3);
		}
	}

	sphereVBuffer_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(VertexDate) * sphereVertex_.size());
	sphereIBuffer_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(uint32_t) * sphereIndex_.size());

	VertexDate* vData = nullptr;
	sphereVBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&vData));
	std::memcpy(vData, sphereVertex_.data(), sizeof(VertexDate) * sphereVertex_.size());

	sphereVbView.BufferLocation = sphereVBuffer_->GetGPUVirtualAddress();
	sphereVbView.SizeInBytes = static_cast<UINT>(sizeof(VertexDate) * sphereVertex_.size());
	sphereVbView.StrideInBytes = static_cast<UINT>(sizeof(VertexDate));

	uint32_t* iData = nullptr;
	sphereIBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&iData));
	std::memcpy(iData, sphereIndex_.data(), sizeof(uint32_t) * sphereIndex_.size());

	sphereIbView.BufferLocation = sphereIBuffer_->GetGPUVirtualAddress();
	sphereIbView.Format = DXGI_FORMAT_R32_UINT;
	sphereIbView.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * sphereIndex_.size());
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
		cylinderVertex_.push_back({ {posBottom.x, posBottom.y, posBottom.z, 1.0f}, {u, 1.0f}, normal });

		// 上
		Vector3 posTop = { cosA * kTopRadius, kHeight, sinA * kTopRadius };
		cylinderVertex_.push_back({ {posTop.x, posTop.y, posTop.z, 1.0f}, {u, 0.0f}, normal });
	}

	// インデックス生成
	for (uint32_t i = 0; i < kCylinderDivide; i++) {
		uint32_t bottom0 = i * 2;
		uint32_t top0 = bottom0 + 1;
		uint32_t bottom1 = bottom0 + 2;
		uint32_t top1 = bottom0 + 3;

		// 三角形1
		cylinderIndex_.push_back(bottom0);
		cylinderIndex_.push_back(top0);
		cylinderIndex_.push_back(bottom1);

		// 三角形2
		cylinderIndex_.push_back(bottom1);
		cylinderIndex_.push_back(top0);
		cylinderIndex_.push_back(top1);
	}

	cylinderVBuffer_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(VertexDate) * cylinderVertex_.size());
	cylinderIBuffer_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(uint32_t) * cylinderIndex_.size());

	VertexDate* vData = nullptr;
	cylinderVBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&vData));
	std::memcpy(vData, cylinderVertex_.data(), sizeof(VertexDate) * cylinderVertex_.size());

	cylinderVbView.BufferLocation = cylinderVBuffer_->GetGPUVirtualAddress();
	cylinderVbView.SizeInBytes = static_cast<UINT>(sizeof(VertexDate) * cylinderVertex_.size());
	cylinderVbView.StrideInBytes = static_cast<UINT>(sizeof(VertexDate));

	uint32_t* iData = nullptr;
	cylinderIBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&iData));
	std::memcpy(iData, cylinderIndex_.data(), sizeof(uint32_t) * cylinderIndex_.size());

	cylinderIbView.BufferLocation = cylinderIBuffer_->GetGPUVirtualAddress();
	cylinderIbView.Format = DXGI_FORMAT_R32_UINT;
	cylinderIbView.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * cylinderIndex_.size());
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
	data["count"] = selectParticleGroup_->insstanceCount_;
	data["Shape"] = static_cast<int>(selectParticleGroup_->shapeType_);
	data["subMode"] = selectParticleGroup_->isSubMode_;
	JsonSerializer::SerializeJsonData(data, ("resource/ParticleGroups/" + currentKey_ + ".json").c_str());
#endif // _DEBUG
}
