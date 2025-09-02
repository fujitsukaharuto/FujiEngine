#include "ParticleManager.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/SRVManager.h"
#include "Engine/Camera/CameraManager.h"
#include "Particle.h"
#include "Math/Random/Random.h"
#include "Engine/DX/FPSKeeper.h"
#include "ImGuiManager/ImGuiManager.h"


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
	InitParticleCS();
	InitGPUEmitter();
	//InitGPUEmitter();
	//InitGPUEmitterTexture();
}

void ParticleManager::Finalize() {
#ifdef _DEBUG
	selectParticleGroup_ = nullptr;
#endif // _DEBUG

	dxcommon_ = nullptr;
	srvManager_ = nullptr;
	camera_ = nullptr;
	for (auto& group : particleGroups_) {
		if (group.second->instancing_) {
			group.second->instancing_->Unmap(0, nullptr);
			group.second->instancingData_ = nullptr;
		}
	}
	particleGroups_.clear();
	for (auto& group : parentParticleGroups_) {
		if (group.second->instancing_) {
			group.second->instancing_->Unmap(0, nullptr);
			group.second->instancingData_ = nullptr;
		}
	}
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
	particleCSInstancing_.Reset();
	freeListIndexResource_.Reset();
	freeListResource_.Reset();
	perViewResource_.Reset();
	perFrameResource_.Reset();
	particleCSMaterial_.Finalize();
	lightning_.reset();

	csEmitters_.clear();
	csEmitterTexs_.clear();
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

	UpdatePerViewData(billboardMatrix);
	UpdateGPUEmitter();
	UpdateGPUEmitterTexture();

	UpdateParticleGroup(billboardMatrix);
	UpdateParentParticleGroup(billboardMatrix);
	UpdateAnimeGroup(billboardMatrix);
}

void ParticleManager::Draw() {
	EmitterTextureDispatch();
	dxcommon_->InsertUAVBarrier(particleCSInstancing_.Get());
	EmitterDispatch();
	dxcommon_->InsertUAVBarrier(particleCSInstancing_.Get());
	UpdateParticleCSDispatch();

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

	DrawParticleCS();
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
		ImGui::Text("count : %d", int(selectParticleGroup_->drawCount_));
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
	if (csEmitters_.size() == 0) return;
	if (ImGui::CollapsingHeader("GPU Particle Emitter")) {
		if (ImGui::TreeNode("ParticleCS Emit Control")) {

			ImGui::Checkbox("isEmit", &csEmitters_[0].isEmit);

			int dragCount = int(csEmitters_[0].emitter->count);
			ImGui::DragInt("emitCount", &dragCount, 1, 0, 100000);
			csEmitters_[0].emitter->count = uint32_t(dragCount);

			ImGui::DragFloat("lifeTime", &csEmitters_[0].emitter->lifeTime, 0.1f, 1.0f, 300.0f);
			ImGui::DragFloat("frequency", &csEmitters_[0].emitter->frequency, 0.1f, 0.0f, 300.0f);

			Vector3 prePos = csEmitters_[0].emitter->translate;
			ImGui::DragFloat3("translate", &csEmitters_[0].emitter->translate.x, 0.1f);
			csEmitters_[0].emitter->prevTranslate = prePos;

			ImGui::DragFloat("radius", &csEmitters_[0].emitter->radius, 0.1f, 0.0f, 300.0f);

			ImGui::SeparatorText("Color");
			ImGui::DragFloat3("colorMax", &csEmitters_[0].emitter->colorMax.x, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat3("colorMin", &csEmitters_[0].emitter->colorMin.x, 0.01f, 0.0f, 1.0f);

			ImGui::SeparatorText("Velocity");
			ImGui::DragFloat3("baseVelocity", &csEmitters_[0].emitter->baseVelocity.x, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("velocityRandMax", &csEmitters_[0].emitter->velocityRandMax, 0.01f, -1.0f, 1.0f);
			ImGui::DragFloat("velocityRandMin", &csEmitters_[0].emitter->velocityRandMin, 0.01f, -1.0f, 1.0f);

			ImGui::Text("DeltaTime1:%f", FPSKeeper::DeltaTime());
			ImGui::Text("DeltaTime2:%f", FPSKeeper::DeltaTimeFrame());
			ImGui::TreePop();
		}
	}
#endif // _DEBUG
}

void ParticleManager::ParticleTexCSDebugGUI() {
#ifdef _DEBUG
	if (csEmitterTexs_.size() == 0) return;
	if (ImGui::CollapsingHeader("GPU ParticleTex Emitter")) {
		if (ImGui::TreeNode("ParticleCS Emit Control")) {

			ImGui::Checkbox("isEmit", &csEmitterTexs_[0].isEmit);

			int dragCount = int(csEmitterTexs_[0].emitter->count);
			ImGui::DragInt("emitCount", &dragCount, 1, 0, 100000);
			csEmitterTexs_[0].emitter->count = uint32_t(dragCount);

			ImGui::DragFloat("lifeTime", &csEmitterTexs_[0].emitter->lifeTime, 0.1f, 1.0f, 300.0f);
			ImGui::DragFloat("frequency", &csEmitterTexs_[0].emitter->frequency, 0.1f, 0.0f, 300.0f);

			ImGui::DragFloat3("translate", &csEmitterTexs_[0].emitter->translate.x, 0.1f);

			ImGui::DragFloat("radius", &csEmitterTexs_[0].emitter->radius, 0.1f, 0.0f, 300.0f);

			ImGui::SeparatorText("Color");
			ImGui::DragFloat3("colorMax", &csEmitterTexs_[0].emitter->colorMax.x, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat3("colorMin", &csEmitterTexs_[0].emitter->colorMin.x, 0.01f, 0.0f, 1.0f);

			ImGui::SeparatorText("Velocity");
			ImGui::DragFloat3("baseVelocity", &csEmitterTexs_[0].emitter->baseVelocity.x, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("velocityRandMax", &csEmitterTexs_[0].emitter->velocityRandMax, 0.01f, -1.0f, 1.0f);
			ImGui::DragFloat("velocityRandMin", &csEmitterTexs_[0].emitter->velocityRandMin, 0.01f, -1.0f, 1.0f);

			ImGui::Text("DeltaTime1:%f", FPSKeeper::DeltaTime());
			ImGui::Text("DeltaTime2:%f", FPSKeeper::DeltaTimeFrame());
			ImGui::TreePop();
		}
	}
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

	ParticleGroup* newGroup = new ParticleGroup();
	newGroup->isSubMode_ = subMode;
	newGroup->shapeType_ = shape;
	newGroup->emitter_.name_ = name;
	newGroup->emitter_.Load(name);

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

	instance->particleGroups_.insert(std::make_pair(name, newGroup));
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
		emit.pos_ = group->emitter_.pos_;
		emit.particleRotate_ = group->emitter_.particleRotate_;
		emit.emitSizeMax_ = group->emitter_.emitSizeMax_;
		emit.emitSizeMin_ = group->emitter_.emitSizeMin_;
		emit.count_ = group->emitter_.count_;
		emit.frequencyTime_ = group->emitter_.frequencyTime_;
		emit.grain_.lifeTime_ = group->emitter_.grain_.lifeTime_;
		emit.grain_.accele_ = group->emitter_.grain_.accele_;
		emit.grain_.speed_ = group->emitter_.grain_.speed_;
		emit.grain_.type_ = group->emitter_.grain_.type_;
		emit.grain_.speedType_ = group->emitter_.grain_.speedType_;
		emit.grain_.rotateType_ = group->emitter_.grain_.rotateType_;
		emit.grain_.colorType_ = group->emitter_.grain_.colorType_;
		emit.grain_.returnPower_ = group->emitter_.grain_.returnPower_;
		emit.grain_.startSize_ = group->emitter_.grain_.startSize_;
		emit.grain_.endSize_ = group->emitter_.grain_.endSize_;
		emit.grain_.isZandX_ = group->emitter_.grain_.isZandX_;
		emit.grain_.isBillBoard_ = group->emitter_.grain_.isBillBoard_;
		emit.grain_.pattern_ = group->emitter_.grain_.pattern_;
		emit.para_.speedx = group->emitter_.para_.speedx;
		emit.para_.speedy = group->emitter_.para_.speedy;
		emit.para_.speedz = group->emitter_.para_.speedz;
		emit.para_.transx = group->emitter_.para_.transx;
		emit.para_.transy = group->emitter_.para_.transy;
		emit.para_.transz = group->emitter_.para_.transz;
		emit.para_.colorMin = group->emitter_.para_.colorMin;
		emit.para_.colorMax = group->emitter_.para_.colorMax;

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

			if (instance->InitEmitParticle(particle, pos, rotate, grain, para)) {
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

			if (instance->InitEmitParticle(particle, pos, rotate, grain, para)) {
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

uint32_t ParticleManager::GetParticleCSEmitterSize() {
	ParticleManager* instance = GetInstance();
	return uint32_t(instance->csEmitters_.size());
}

ParticleManager::GPUParticleEmitter& ParticleManager::GetParticleCSEmitter(int index) {
	ParticleManager* instance = GetInstance();
	assert(index >= 0 && index < instance->csEmitters_.size());
	return instance->csEmitters_[index];
}

ParticleManager::GPUParticleEmitterTexture& ParticleManager::GetParticleCSEmitterTexture(int index) {
	ParticleManager* instance = GetInstance();
	assert(index >= 0 && index < instance->csEmitterTexs_.size());
	return instance->csEmitterTexs_[index];
}

int ParticleManager::InitGPUEmitter() {
	GPUParticleEmitter CSEmitter;
	CSEmitter.isEmit = false;

	CSEmitter.emitterResource = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), (sizeof(EmitterSphere)));
	CSEmitter.emitterResource->Map(0, nullptr, reinterpret_cast<void**>(&CSEmitter.emitter));
	CSEmitter.emitter->count = 500;
	CSEmitter.emitter->lifeTime = 60.0f;
	CSEmitter.emitter->frequency = 0.5f;
	CSEmitter.emitter->frequencyTime = 0.0f;
	CSEmitter.emitter->translate = Vector3(0.0f, 0.0f, 0.0f);
	CSEmitter.emitter->scale = Vector3(0.1f, 0.1f, 0.1f);
	CSEmitter.emitter->radius = 2.5f;
	CSEmitter.emitter->emit = 0;
	CSEmitter.emitter->colorMax = { 1.0f,1.0f,1.0f };
	CSEmitter.emitter->colorMin = { 0.0f,0.0f,0.0f };
	CSEmitter.emitter->baseVelocity = { 0.0f,0.0f,0.0f };
	CSEmitter.emitter->velocityRandMax = 0.0f;
	CSEmitter.emitter->velocityRandMin = 0.0f;

	CSEmitter.emitter->prevTranslate = Vector3(0.0f, 0.0f, 0.0f);

	CSEmitter.emitterIndex = csEmitterIndex_;
	csEmitters_.push_back(CSEmitter);
	int result = csEmitterIndex_;
	csEmitterIndex_++;
	return result;
}

int ParticleManager::InitGPUEmitterTexture() {
	GPUParticleEmitterTexture CSEmitter;
	CSEmitter.isEmit = true;

	CSEmitter.emitterResource = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), (sizeof(EmitterSphere)));
	CSEmitter.emitterResource->Map(0, nullptr, reinterpret_cast<void**>(&CSEmitter.emitter));
	CSEmitter.emitter->count = 500;
	CSEmitter.emitter->lifeTime = 10.0f;
	CSEmitter.emitter->frequency = 2.0f;
	CSEmitter.emitter->frequencyTime = 0.0f;
	CSEmitter.emitter->translate = Vector3(0.0f, 0.5f, 0.0f);
	CSEmitter.emitter->radius = 10.0f;
	CSEmitter.emitter->emit = 0;
	CSEmitter.emitter->colorMax = { 1.0f,0.0f,1.0f };
	CSEmitter.emitter->colorMin = { 0.0f,0.0f,0.0f };
	CSEmitter.emitter->baseVelocity = { 0.0f,0.0f,0.0f };
	CSEmitter.emitter->velocityRandMax = 0.01f;
	CSEmitter.emitter->velocityRandMin = 0.0f;

	CSEmitter.textureForEmit = TextureManager::GetInstance()->LoadTexture("magicCircle.png");

	CSEmitter.emitterIndex = csEmitterTexIndex_;
	csEmitterTexs_.push_back(CSEmitter);
	int result = csEmitterTexIndex_;
	csEmitterTexIndex_++;
	return result;
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

		int particleCount = 0;
		group->drawCount_ = 0;
		for (auto& particle : group->particles_) {
			if (LifeUpdate(particle)) {
				continue;
			}

			ParticleSizeUpdate(particle);

			Matrix4x4 worldViewProjectionMatrix;
			Matrix4x4 worldMatrix = MakeIdentity4x4();
			SRTUpdate(particle);
			Billboard(particle, worldMatrix, billboardMatrix, MakeIdentity4x4());

			if (camera_) {
				const Matrix4x4& viewProjectionMatrix = camera_->GetViewProjectionMatrix();
				worldViewProjectionMatrix = Multiply(worldMatrix, viewProjectionMatrix);
			} else {
				worldViewProjectionMatrix = worldMatrix;
			}

			group->instancingData_[particleCount].World = worldMatrix;
			group->instancingData_[particleCount].WVP = worldViewProjectionMatrix;
			group->instancingData_[particleCount].color = particle.color_;
			group->instancingData_[particleCount].uvTrans = particle.uvTrans_;
			group->instancingData_[particleCount].uvScale = particle.uvScale_;

			particleCount++;
			group->drawCount_++;
		}
	}
}

void ParticleManager::UpdateParentParticleGroup(const Matrix4x4& billboardMatrix) {
	for (auto& groupPair : parentParticleGroups_) {
		ParentParticleGroup* group = groupPair.second.get();
		for (auto& particle : group->particles_) {
			if (!particle.isLive_) {
				continue;
			}
			if (!group->emitter_->GetIsUpdatedMatrix()) {
				group->emitter_->worldMatrix_ = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, group->emitter_->pos_);
				if (group->emitter_->HaveParent()) {
					const Matrix4x4& parentWorldMatrix = group->emitter_->GetParentMatrix();
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
					group->emitter_->worldMatrix_ = Multiply(group->emitter_->worldMatrix_, noScaleParentMatrix);
				}
			}
			break;
		}

		int particleCount = 0;
		group->drawCount_ = 0;
		for (auto& particle : group->particles_) {
			if (LifeUpdate(particle)) {
				continue;
			}

			ParticleSizeUpdate(particle);

			Matrix4x4 worldViewProjectionMatrix;
			Matrix4x4 worldMatrix = MakeIdentity4x4();
			Matrix4x4 parentRotate = MakeIdentity4x4();
			SRTUpdate(particle);
			if (particle.isParentRotate_) {
				parentRotate = group->emitter_->worldMatrix_;
				// スケールを除去する（上のコードと同様）
				Vector3 xAxis = { parentRotate.m[0][0], parentRotate.m[1][0], parentRotate.m[2][0] };
				Vector3 yAxis = { parentRotate.m[0][1], parentRotate.m[1][1], parentRotate.m[2][1] };
				Vector3 zAxis = { parentRotate.m[0][2], parentRotate.m[1][2], parentRotate.m[2][2] };

				float xLen = Vector3::Length(xAxis);
				float yLen = Vector3::Length(yAxis);
				float zLen = Vector3::Length(zAxis);

				for (int i = 0; i < 3; ++i) {
					parentRotate.m[i][0] /= xLen;
					parentRotate.m[i][1] /= yLen;
					parentRotate.m[i][2] /= zLen;
				}
			}
			Billboard(particle, worldMatrix, billboardMatrix, parentRotate);

			if (particle.isParent_) {
				// 親行列のスケール・回転を取り除いた「平行移動のみマトリクス」を作る
				Vector3 parentTranslate = { group->emitter_->worldMatrix_.m[3][0],
											group->emitter_->worldMatrix_.m[3][1],
											group->emitter_->worldMatrix_.m[3][2] };
				Matrix4x4 parentTranslateMatrix = MakeTranslateMatrix(parentTranslate);

				worldMatrix = Multiply(worldMatrix, parentTranslateMatrix);
			}
			if (camera_) {
				const Matrix4x4& viewProjectionMatrix = camera_->GetViewProjectionMatrix();
				worldViewProjectionMatrix = Multiply(worldMatrix, viewProjectionMatrix);
			} else {
				worldViewProjectionMatrix = worldMatrix;
			}

			group->instancingData_[particleCount].World = worldMatrix;
			group->instancingData_[particleCount].WVP = worldViewProjectionMatrix;
			group->instancingData_[particleCount].color = particle.color_;
			group->instancingData_[particleCount].uvTrans = particle.uvTrans_;
			group->instancingData_[particleCount].uvScale = particle.uvScale_;

			particleCount++;
			group->drawCount_++;
		}
		group->emitter_->SetIsUpdatedMatrix(false);
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
		if (group->shapeType_ == ShapeType::Lightning) continue;

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
		if (group->shapeType_ != ShapeType::Lightning) continue;

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
		if (group->shapeType_ == ShapeType::Lightning) continue;

		ShapeTypeCommand(group->shapeType_);
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(1, srvManager_->GetGPUDescriptorHandle(group->srvIndex_));
		dxcommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, group->material_.GetMaterialResource()->GetGPUVirtualAddress());
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, group->material_.GetTexture()->gpuHandle);
		ShapeTypeDrawCommand(group->shapeType_, group->drawCount_);
	}
	for (auto& groupPair : parentParticleGroups_) {
		ParentParticleGroup* group = groupPair.second.get();
		if (group->drawCount_ == 0) continue;
		if (group->shapeType_ != ShapeType::Lightning) continue;

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

void ParticleManager::InitParticleCS() {
	particleCSInsstanceCount_ = numParticles;
	particleCSInstancing_= dxcommon_->CreateUAVResource(dxcommon_->GetDevice(), (sizeof(ParticleCS) * particleCSInsstanceCount_));

	particleCSMaterial_.SetTextureNamePath("redCircle.png");
	particleCSMaterial_.CreateMaterial();

	uint32_t particleCSSRVIndex = srvManager_->Allocate();
	uint32_t particleCSUAVIndex = srvManager_->Allocate();
	srvManager_->CreateStructuredSRV(particleCSSRVIndex, particleCSInstancing_.Get(), particleCSInsstanceCount_, sizeof(ParticleCS));
	srvManager_->CreateStructuredUAV(particleCSUAVIndex, particleCSInstancing_.Get(), particleCSInsstanceCount_, sizeof(ParticleCS));
	particleCSSRVHandle_.first = srvManager_->GetCPUDescriptorHandle(particleCSSRVIndex);
	particleCSSRVHandle_.second = srvManager_->GetGPUDescriptorHandle(particleCSSRVIndex);
	particleCSUAVHandle_.first = srvManager_->GetCPUDescriptorHandle(particleCSUAVIndex);
	particleCSUAVHandle_.second = srvManager_->GetGPUDescriptorHandle(particleCSUAVIndex);

	freeListIndexResource_ = dxcommon_->CreateUAVResource(dxcommon_->GetDevice(), (sizeof(int32_t)));
	uint32_t freeCountUAVIndex = srvManager_->Allocate();
	srvManager_->CreateStructuredUAV(freeCountUAVIndex, freeListIndexResource_.Get(), 1, sizeof(int32_t));
	freeListIndexUAVHandle_.first = srvManager_->GetCPUDescriptorHandle(freeCountUAVIndex);
	freeListIndexUAVHandle_.second = srvManager_->GetGPUDescriptorHandle(freeCountUAVIndex);

	freeListResource_ = dxcommon_->CreateUAVResource(dxcommon_->GetDevice(), (sizeof(uint32_t)* particleCSInsstanceCount_));
	uint32_t freeListUAVIndex = srvManager_->Allocate();
	srvManager_->CreateStructuredUAV(freeListUAVIndex, freeListResource_.Get(), particleCSInsstanceCount_, sizeof(uint32_t));
	freeListUAVHandle_.first = srvManager_->GetCPUDescriptorHandle(freeListUAVIndex);
	freeListUAVHandle_.second = srvManager_->GetGPUDescriptorHandle(freeListUAVIndex);

	srvManager_->SetDescriptorHeap();
	dxcommon_->GetPipelineManager()->SetCSPipeline(Pipe::InitParticleCS);
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(0, particleCSUAVHandle_.second);
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(1, freeListIndexUAVHandle_.second);
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(2, freeListUAVHandle_.second);
	int dispatchCount = (numParticles + threadsPerGroup - 1) / threadsPerGroup;
	dxcommon_->GetCommandList()->Dispatch(dispatchCount,1,1);
	dxcommon_->CommandExecution();

	perViewResource_ =dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), (sizeof(PerView)));
	perViewResource_->Map(0, nullptr, reinterpret_cast<void**>(&perViewData_));
	perViewData_->viewProjection = MakeIdentity4x4();
	perViewData_->billboardMatrix = MakeIdentity4x4();

	perFrameResource_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), (sizeof(PerFrame)));
	perFrameResource_->Map(0, nullptr, reinterpret_cast<void**>(&perFrameData_));
	perFrameData_->time = 0.0f;
	perFrameData_->deltaTime = 0.0f;
}

void ParticleManager::UpdatePerViewData(const Matrix4x4& billboardMatrix) {
	perViewData_->viewProjection = camera_->GetViewProjectionMatrix();
	perViewData_->billboardMatrix = billboardMatrix;

	perFrameData_->deltaTime = FPSKeeper::DeltaTime();
	perFrameData_->time += perFrameData_->deltaTime;
	if (perFrameData_->time > 420.0f) {
		perFrameData_->time = 0.0f;
	}
}

void ParticleManager::DrawParticleCS() {
	dxcommon_->GetDXCommand()->SetViewAndscissor();
	dxcommon_->GetPipelineManager()->SetPipeline(Pipe::particleCS);
	dxcommon_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vbView);
	dxcommon_->GetCommandList()->IASetIndexBuffer(&ibView);

	dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, particleCSSRVHandle_.second);
	dxcommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, perViewResource_->GetGPUVirtualAddress());
	dxcommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, particleCSMaterial_.GetMaterialResource()->GetGPUVirtualAddress());
	dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(3, particleCSMaterial_.GetTexture()->gpuHandle);

	dxcommon_->GetCommandList()->DrawIndexedInstanced(6, particleCSInsstanceCount_, 0, 0, 0);
}

void ParticleManager::UpdateGPUEmitter() {
	for (int i = 0; i < csEmitters_.size(); i++) {
		auto& emitter = csEmitters_[i];
		if (emitter.isEmit) {
			emitter.emitter->frequencyTime += FPSKeeper::DeltaTime();
			if (emitter.emitter->frequency <= emitter.emitter->frequencyTime) {
				emitter.emitter->frequencyTime -= emitter.emitter->frequency;
				emitter.emitter->emit = 1;
			} else {
				emitter.emitter->emit = 0;
			}
		} else {
			emitter.emitter->emit = 0;
			emitter.emitter->frequencyTime = 0.0f;
		}
	}
}

void ParticleManager::UpdateParticleCSDispatch() {
	dxcommon_->GetPipelineManager()->SetCSPipeline(Pipe::UpdateParticleCS);
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(0, particleCSUAVHandle_.second);
	dxcommon_->GetCommandList()->SetComputeRootConstantBufferView(1, perFrameResource_->GetGPUVirtualAddress());
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(2, freeListIndexUAVHandle_.second);
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(3, freeListUAVHandle_.second);
	int dispatchCount = (numParticles + threadsPerGroup - 1) / threadsPerGroup;
	dxcommon_->GetCommandList()->Dispatch(dispatchCount, 1, 1);
}

void ParticleManager::EmitterDispatch() {
	for (int i = 0; i < csEmitters_.size(); i++) {
		dxcommon_->GetPipelineManager()->SetCSPipeline(Pipe::EmitParticleCS);
		dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(0, particleCSUAVHandle_.second);
		dxcommon_->GetCommandList()->SetComputeRootConstantBufferView(1, csEmitters_[i].emitterResource->GetGPUVirtualAddress());
		dxcommon_->GetCommandList()->SetComputeRootConstantBufferView(2, perFrameResource_->GetGPUVirtualAddress());
		dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(3, freeListIndexUAVHandle_.second);
		dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(4, freeListUAVHandle_.second);
		if (csEmitters_[i].emitter->count == 0) continue;
		int dispatchCount = (csEmitters_[i].emitter->count + threadGroupSize_ - 1) / threadGroupSize_;
		dxcommon_->GetCommandList()->Dispatch(dispatchCount, 1, 1);
	}
}

void ParticleManager::UpdateGPUEmitterTexture() {
	for (int i = 0; i < csEmitterTexs_.size(); i++) {
		auto& emitter = csEmitterTexs_[i];
		if (emitter.isEmit) {
			emitter.emitter->frequencyTime += FPSKeeper::DeltaTime();
			if (emitter.emitter->frequency <= emitter.emitter->frequencyTime) {
				emitter.emitter->frequencyTime -= emitter.emitter->frequency;
				emitter.emitter->emit = 1;
			} else {
				emitter.emitter->emit = 0;
			}
		} else {
			emitter.emitter->emit = 0;
			emitter.emitter->frequencyTime = 0.0f;
		}
	}
}

void ParticleManager::EmitterTextureDispatch() {
	for (int i = 0; i < csEmitterTexs_.size(); i++) {
		dxcommon_->GetPipelineManager()->SetCSPipeline(Pipe::EmitTexParticleCS);
		dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(0, particleCSUAVHandle_.second);
		dxcommon_->GetCommandList()->SetComputeRootConstantBufferView(1, csEmitterTexs_[i].emitterResource->GetGPUVirtualAddress());
		dxcommon_->GetCommandList()->SetComputeRootConstantBufferView(2, perFrameResource_->GetGPUVirtualAddress());
		dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(3, freeListIndexUAVHandle_.second);
		dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(4, freeListUAVHandle_.second);
		dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(5, csEmitterTexs_[i].textureForEmit->gpuHandle);
		if (csEmitterTexs_[i].emitter->count == 0) continue;
		int dispatchCountX = (int(csEmitterTexs_[i].textureForEmit->meta.width)  + 32 - 1) / 32;
		int dispatchCountY = (int(csEmitterTexs_[i].textureForEmit->meta.height) + 32 - 1) / 32;
		dxcommon_->GetCommandList()->Dispatch(dispatchCountX, dispatchCountY, 1);
	}
}


bool ParticleManager::LifeUpdate(Particle& particle) {
	if (particle.lifeTime_ <= 0) {
		particle.isLive_ = false;
		return true;
	}
	if (!particle.isLive_) {
		return true;
	}

	particle.lifeTime_--;
	return false;
}

void ParticleManager::ParticleSizeUpdate(Particle& particle) {
	SizeType sizeType = SizeType(particle.type_);
	float t = (1.0f - float(float(particle.lifeTime_) / float(particle.startLifeTime_)));

	if (particle.isColorFade_) {
		particle.color_.w = Lerp(particle.startAlpha_, 0.0f, t * t);
	}
	if (particle.isAutoUVMove_) {
		particle.uvTrans_ += particle.autoUVSpeed_;
	}

	switch (sizeType) {
	case SizeType::kNormal:
		break;
	case SizeType::kShift:

		particle.scale.x = Lerp(particle.startSize_.x, particle.endSize_.x, t);
		particle.scale.y = Lerp(particle.startSize_.y, particle.endSize_.y, t);
		if (particle.isZandX_) {
			particle.scale.z = Lerp(particle.startSize_.x, particle.endSize_.x, t);
		}

		break;
	case SizeType::kSin:

		Vector2 minSize = particle.startSize_; // 最小値
		Vector2 maxSize = particle.endSize_; // 最大値

		if (minSize.x > maxSize.x) {
			std::swap(minSize.x, maxSize.x); // minとmaxを交換
		}
		if (minSize.y > maxSize.y) {
			std::swap(minSize.y, maxSize.y); // minとmaxを交換
		}

		Vector2 sizeSin = minSize + (maxSize - minSize) * 0.5f * (1.0f + sin(particle.lifeTime_));

		particle.scale.x = sizeSin.x;
		particle.scale.y = sizeSin.y;

		break;
	}
}

void ParticleManager::SRTUpdate(Particle& particle) {
	if (particle.rotateType_ == static_cast<int>(RotateType::kRandomR)) {
		if (particle.isContinuouslyRotate_) {
			particle.rotate += Random::GetVector3({ -0.2f,0.2f }, { -0.2f,0.2f }, { -0.2f,0.2f }) * FPSKeeper::DeltaTime();
		}
	}

	particle.speed_ += particle.accele_ * FPSKeeper::DeltaTime();
	particle.translate += particle.speed_ * FPSKeeper::DeltaTime();
}

void ParticleManager::Billboard(Particle& particle, Matrix4x4& worldMatrix, const Matrix4x4& billboardMatrix, const Matrix4x4& rotate) {
	if (!particle.isBillBoard_) {
		worldMatrix = MakeAffineMatrix(particle.scale, particle.rotate, particle.translate);
	}
	if (particle.isBillBoard_) {
		switch (particle.pattern_) {
		case BillBoardPattern::kXYZBillBoard: {
			Vector3 possition = particle.translate;
			if (particle.isParentRotate_) {
				possition = TransformNormal(possition, rotate);
			}
			worldMatrix = Multiply(MakeScaleMatrix(particle.scale), billboardMatrix);
			worldMatrix = Multiply(worldMatrix, MakeTranslateMatrix(possition));
			break;
		}
		case BillBoardPattern::kXBillBoard: {

			Matrix4x4 xBillboardMatrix;
			xBillboardMatrix = billboardMatrix;
			xBillboardMatrix.m[1][0] = 0.0f; // Y軸成分をゼロにする
			xBillboardMatrix.m[2][0] = 0.0f; // Z軸成分をゼロにする

			worldMatrix = Multiply(MakeScaleMatrix(particle.scale), MakeRotateXYZMatrix({ 0.0f,particle.rotate.y,particle.rotate.z }));
			worldMatrix = Multiply(worldMatrix, xBillboardMatrix);
			worldMatrix = Multiply(worldMatrix, MakeTranslateMatrix(particle.translate));
			break;
		}
		case BillBoardPattern::kYBillBoard: {
			if (camera_) {
				Matrix4x4 yBillboardMatrix = billboardMatrix;
				yBillboardMatrix.m[0][1] = 0.0f; // X軸成分をゼロにする
				yBillboardMatrix.m[2][1] = 0.0f; // Z軸成分をゼロにする

				worldMatrix = Multiply(MakeScaleMatrix(particle.scale), MakeRotateXYZMatrix({ particle.rotate.x,0.0f,particle.rotate.z }));
				worldMatrix = Multiply(worldMatrix, yBillboardMatrix);
				worldMatrix = Multiply(worldMatrix, MakeTranslateMatrix(particle.translate));
			}
			break;
		}
		case BillBoardPattern::kZBillBoard: {

			Matrix4x4 zBillboardMatrix = billboardMatrix;
			zBillboardMatrix.m[0][2] = 0.0f; // X軸成分をゼロにする
			zBillboardMatrix.m[1][2] = 0.0f; // Y軸成分をゼロにする

			worldMatrix = Multiply(MakeScaleMatrix(particle.scale), MakeRotateXYZMatrix({ particle.rotate.x,particle.rotate.y,0.0f }));
			worldMatrix = Multiply(worldMatrix, zBillboardMatrix);
			worldMatrix = Multiply(worldMatrix, MakeTranslateMatrix(particle.translate));

			break;
		}
		case BillBoardPattern::kXYBillBoard: {
			Matrix4x4 xyBillboardMatrix = billboardMatrix;
			xyBillboardMatrix.m[0][1] = 0.0f; // X軸成分をゼロにする
			xyBillboardMatrix.m[2][1] = 0.0f; // Z軸成分をゼロにする
			xyBillboardMatrix.m[1][0] = 0.0f; // Y軸成分をゼロにする
			xyBillboardMatrix.m[2][0] = 0.0f; // Z軸成分をゼロにする

			worldMatrix = Multiply(MakeScaleMatrix(particle.scale), MakeRotateXYZMatrix({ particle.rotate.x,0.0f,particle.rotate.z }));
			worldMatrix = Multiply(worldMatrix, xyBillboardMatrix);
			worldMatrix = Multiply(worldMatrix, MakeTranslateMatrix(particle.translate));

			break;
		}
		default:
			break;
		}
	}
}

bool ParticleManager::InitEmitParticle(Particle& particle, const Vector3& pos, const Vector3& rotate, const Particle& grain, const RandomParametor& para) {

	if (particle.isLive_ == false) {
		particle.translate = grain.translate;
		particle.scale = grain.scale;
		particle.rotate = grain.rotate;
		particle.translate = Random::GetVector3(para.transx, para.transy, para.transz);
		particle.translate += pos;
		particle.scale = { grain.startSize_.x + para.addRandomSize.x,grain.startSize_.y + para.addRandomSize.y,1.0f };
		if (grain.speedType_ == static_cast<int>(SpeedType::kCenter)) {
			particle.speed_ = grain.speed_;
		} else {
			particle.speed_ = Random::GetVector3(para.speedx, para.speedy, para.speedz);
		}
		particle.returnPower_ = grain.returnPower_;

		particle.rotateType_ = grain.rotateType_;
		particle.isContinuouslyRotate_ = grain.isContinuouslyRotate_;
		Vector3 veloSpeed = particle.speed_.Normalize();
		Vector3 cameraR{};
		Vector3 defo = { 0.0f,1.0f,0.0f };
		Vector3 angleDToD{};
		Matrix4x4 rotateCamera;
		Matrix4x4 dToD;

		switch (particle.rotateType_) {
		case static_cast<int>(RotateType::kUsually):
			particle.rotate = rotate;
			break;
		case static_cast<int>(RotateType::kVelocityR):

			veloSpeed = particle.speed_.Normalize();

			// カメラの回転を考慮して速度ベクトルを変換
			cameraR = CameraManager::GetInstance()->GetCamera()->transform.rotate;
			rotateCamera = MakeRotateXYZMatrix(-cameraR);
			veloSpeed = TransformNormal(veloSpeed, rotateCamera);

			defo = TransformNormal(defo, rotateCamera);
			dToD = DirectionToDirection(defo, veloSpeed.Normalize());
			angleDToD = ExtractEulerAngles(dToD);
			particle.rotate = angleDToD;

			break;
		case static_cast<int>(RotateType::kRandomR):
			particle.rotate = Random::GetVector3({ -3.0f,3.0f }, { -3.0f,3.0f }, { -3.0f,3.0f });
			break;
		}

		particle.lifeTime_ = grain.lifeTime_;
		particle.startLifeTime_ = particle.lifeTime_;
		particle.isBillBoard_ = grain.isBillBoard_;
		particle.pattern_ = grain.pattern_;
		particle.colorType_ = grain.colorType_;
		particle.isColorFade_ = grain.isColorFade_;
		particle.uvTrans_ = grain.uvTrans_;
		particle.uvScale_ = grain.uvScale_;
		particle.isAutoUVMove_ = grain.isAutoUVMove_;
		particle.autoUVSpeed_ = Vector2((Random::GetFloat(para.autoUVMin.x, para.autoUVMax.x)), (Random::GetFloat(para.autoUVMin.y, para.autoUVMax.y)));
		switch (particle.colorType_) {
		case static_cast<int>(ColorType::kDefault):
			particle.color_ = para.colorMax;
			break;
		case static_cast<int>(ColorType::kRandom):
			particle.color_.x = Random::GetFloat(para.colorMin.x, para.colorMax.x);
			particle.color_.y = Random::GetFloat(para.colorMin.y, para.colorMax.y);
			particle.color_.z = Random::GetFloat(para.colorMin.z, para.colorMax.z);
			particle.color_.w = Random::GetFloat(para.colorMin.w, para.colorMax.w);
			break;
		}
		particle.startAlpha_ = particle.color_.w;

		SpeedType type = SpeedType(grain.speedType_);
		switch (type) {
		case SpeedType::kConstancy:
			particle.accele_ = Vector3{ 0.0f,0.0f,0.0f };
			break;
		case SpeedType::kChange:
			particle.accele_ = grain.accele_;
			break;
		case SpeedType::kReturn:
			particle.accele_ = (particle.speed_) * grain.returnPower_;
			break;
		case SpeedType::kCenter:
			particle.accele_ = Vector3{ 0.0f,0.0f,0.0f };
			break;
		}

		particle.type_ = grain.type_;
		particle.startSize_ = grain.startSize_ + para.addRandomSize;
		particle.endSize_ = grain.endSize_ + para.addRandomSize;
		particle.isZandX_ = grain.isZandX_;
		particle.isParent_ = grain.isParent_;
		particle.isParentRotate_ = grain.isParentRotate_;

		particle.isLive_ = true;
		return true;
	}
	return false;
}
