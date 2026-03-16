#include "ParticleManager.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/DX12Helper.h"
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

void ParticleManager::Finalize() {
#ifdef _DEBUGMODE
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

	dxcommon_->GetDXCommand()->SetViewAndScissor(MyWin::kWindowWidth, MyWin::kWindowHeight);
	dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Normal);
	dxcommon_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	for (auto& groupPair : animeGroups_) {
		AnimeGroup* group = groupPair.second.get();
		for (int i = 0; i < group->objects_.size(); i++) {
			if (!group->isLive_[i]) continue;
			group->objects_[i]->AnimeDraw();
		}
	}

	dxcommon_->GetDXCommand()->SetViewAndScissor(MyWin::kWindowWidth, MyWin::kWindowHeight);
	dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Particle);
	preType_ = BlendType::ADD;
	dxcommon_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &plane_.vbView);
	dxcommon_->GetCommandList()->IASetIndexBuffer(&plane_.ibView);
	DrawParticleGroup();
	DrawParentParticleGroup();
}

void ParticleManager::SelectParticleUpdate() {
#ifdef _DEBUGMODE
	if (selectParticleGroup_) {
		selectParticleGroup_->emitter_.Emit();
	}
#endif // _DEBUG
}

void ParticleManager::SelectEmitterSizeDraw() {
#ifdef _DEBUGMODE
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

	newGroup->first = fileName;
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

void ParticleManager::Emit(IParticleGroup* group, const Vector3& pos, const Vector3& rotate, const Particle& grain, const RandomParameter& para, uint32_t count) {
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

void ParticleManager::ParentEmit(IParticleGroup* group, const Vector3& pos, const Vector3& rotate, const Particle& grain, const RandomParameter& para, uint32_t count) {
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

void ParticleManager::EmitAnime(const std::string& name, const Vector3& pos, const AnimeData& data, const RandomParameter& para, uint32_t count) {
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
				group->objects_[i]->GetTransform().translate = Random::GetVector3(para.transx, para.transy, para.transz);
				group->objects_[i]->GetTransform().translate += pos;
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

				group->objects_[i]->SetTexture(group->first);
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

MeshSurfaceEmitter& ParticleManager::GetParticleCSEmitterSurface(int index) {
	ParticleManager* instance = GetInstance();
	return instance->gpuParticleSystem_->GetParticleCSEmitterSurface(index);
}

void ParticleManager::SetIsStopped(bool flag) {
	ParticleManager* instance = GetInstance();
	instance->isStopped_ = flag;
}

void ParticleManager::ResetCSEmitters() {
	gpuParticleSystem_->ResetEmitters();
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
	if (isParent) { // ペアレントするかどうか
		auto iterator = parentParticleGroups_.find(name);
		if (iterator != parentParticleGroups_.end()) {
			return;
		}

		auto newGroup = std::make_unique<ParentParticleGroup>();
		newGroup->emitter_ = std::make_unique<ParticleEmitter>();
		newGroup->shapeType_ = shape;
		newGroup->emitter_->name_ = name;
		newGroup->emitter_->Load(name);

		newGroup->instanceCount_ = count; // 出す数
		uint32_t max = newGroup->instanceCount_;
		for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
			newGroup->instancing_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), (sizeof(TransformationParticleMatrix) * newGroup->instanceCount_));
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

		newGroup->instanceCount_ = count; // 出す数
		for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
			newGroup->instancing_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(),
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

		newGroup->emitter_.SetGroup(newGroup.get());
		particleGroups_.insert(std::make_pair(name, std::move(newGroup)));
	}
}

void ParticleManager::UpdateParticleGroup(const Matrix4x4& billboardMatrix) {
	for (auto& groupPair : particleGroups_) {
		ParticleGroup* group = groupPair.second.get();
#ifdef _DEBUGMODE
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
	for (auto& groupPair : animeGroups_) { // アニメーショングループ
		AnimeGroup* group = groupPair.second.get();
		for (int i = 0; i < group->objects_.size(); i++) {
			if (group->lifeTime[i] <= 0) {
				group->isLive_[i] = false;
				continue;
			}

			group->lifeTime[i] -= FPSKeeper::DeltaTimeFrame();
			group->animeTime[i] += FPSKeeper::DeltaTimeFrame();

			for (auto& animeChange : group->anime_) { // 切り替え
				if (group->animeTime[i] >= animeChange.second * FPSKeeper::DeltaTimeFrame()) {
					group->objects_[i]->SetTexture(animeChange.first);
				}
			}

			SizeType sizeType = SizeType(group->type);
			float t = (1.0f - float(float(group->lifeTime[i]) / float(group->startLifeTime_[i])));
			switch (sizeType) {//サイズのタイプ
			case SizeType::kNormal:
				break;
			case SizeType::kShift:

				group->objects_[i]->GetTransform().scale.x = Lerp(group->startSize.x, group->endSize.x, t);
				group->objects_[i]->GetTransform().scale.y = Lerp(group->startSize.y, group->endSize.y, t);

				break;
			}

			group->speed[i] += group->accele[i] * FPSKeeper::DeltaTimeFrame();

			group->objects_[i]->GetTransform().translate += group->speed[i] * FPSKeeper::DeltaTimeFrame();
			group->objects_[i]->SetBillboardMat(billboardMatrix);
		}
	}
}

void ParticleManager::DrawParticleGroup() {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	for (auto& groupPair : particleGroups_) {// 通常パーティクルグループ
		ParticleGroup* group = groupPair.second.get();
		if (group->drawCount_ == 0) continue;
		if (group->shapeType_ == ShapeType::LIGHTNING) continue;
		if (preType_ != group->type_) {
			switch (group->type_) {// ブレンドモードによって切り替え
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
		ShapeTypeDrawCommand(group->shapeType_, group->drawCount_);// ShapeTypeでDrawするように

		preType_ = group->type_;
	}


	for (auto& groupPair : particleGroups_) {// ペアレントパーティクルグループ
		ParticleGroup* group = groupPair.second.get();
		if (group->drawCount_ == 0) continue;
		if (group->shapeType_ != ShapeType::LIGHTNING) continue;
		if (preType_ != group->type_) {// ブレンドモードによって切り替え
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
		lightning_->MeshDraw(&group->material_, group->drawCount_);// ライトニングの描画

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
	switch (type) {// 形状
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

void ParticleManager::SaveGroupData() {
#ifdef _DEBUGMODE
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
