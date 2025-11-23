#include "SphereEmitter.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/SRVManager.h"
#include "ImGuiManager/ImGuiManager.h"

SphereEmitter::SphereEmitter(DXCom* dx) {
	resource_ = dx->CreateBufferResource(dx->GetDevice(), sizeof(EmitterSphere));
	resource_->Map(0, nullptr, reinterpret_cast<void**>(&data_));
	*data_ = {};
	data_->count = 300;
	data_->lifeTime = 60.0f;
	data_->frequency = 0.5f;
	data_->radius = 2.5f;
	data_->scale = Vector3(0.1f, 0.1f, 0.1f);
	data_->emit = 0;
	data_->colorMax = { 1.0f,1.0f,1.0f };
	data_->colorMin = { 0.0f,0.0f,0.0f };
	data_->isDistance = 1;
	data_->rotation = Quaternion::IdentityQuaternion();
}

void SphereEmitter::Update(float deltaTime) {
	if (!isEmit_) {
		data_->emit = 0;
		data_->frequencyTime = 0.0f;
		return;
	}

	data_->frequencyTime += deltaTime;
	if (data_->frequency <= data_->frequencyTime) {
		data_->frequencyTime = 0.0f;
		data_->emit = 1;
	} else {
		data_->emit = 0;
	}
}

void SphereEmitter::Dispatch(ID3D12GraphicsCommandList* cmd,
	DXCom* dx, [[maybe_unused]] SRVManager* srv, const ParticleCSHandles& shared) {
	if (!isEmit_ || data_->count == 0) return;
	dx->GetPipelineManager()->SetCSPipeline(Pipe::EmitParticleCS);
	cmd->SetComputeRootDescriptorTable(0, shared.particleCSUAVHandle);
	cmd->SetComputeRootDescriptorTable(3, shared.freeListIndexUAVHandle);
	cmd->SetComputeRootDescriptorTable(4, shared.freeListUAVHandle);
	cmd->SetComputeRootDescriptorTable(5, shared.freeListTailIndexUAVHandle);
	cmd->SetComputeRootConstantBufferView(2, shared.perFrameCBV);
	cmd->SetComputeRootConstantBufferView(1, resource_->GetGPUVirtualAddress());
	cmd->Dispatch((data_->count + 1024 - 1) / 1024, 1, 1);
}

void SphereEmitter::DebugGUI() {
#ifdef _DEBUG
	if (ImGui::TreeNode("ParticleCS Emit Control")) {
		ImGui::Checkbox("IsEmit", &isEmit_);

		int dragCount = int(data_->count);
		ImGui::DragInt("EmitCount", &dragCount, 1, 0, 100000);
		data_->count = uint32_t(dragCount);

		ImGui::DragFloat("LifeTime", &data_->lifeTime, 0.1f, 1.0f, 300.0f);
		ImGui::DragFloat("Frequency", &data_->frequency, 0.1f, 0.0f, 300.0f);

		Vector3 prePos = data_->translate;
		ImGui::DragFloat3("Translate", &data_->translate.x, 0.1f);
		data_->prevTranslate = prePos;
		ImGui::DragFloat3("PreTranslate", &data_->prevTranslate.x, 0.1f);
		bool isDistance = bool(data_->isDistance);
		ImGui::Checkbox("IsDistance", &isDistance);
		data_->isDistance = uint32_t(isDistance);

		ImGui::DragFloat("Scale", &data_->radius, 0.1f, 0.01f, 300.0f);

		int shapeType = int(data_->emitShapeType);
		ImGui::DragInt("EmitShapeType", &shapeType, 0.1f, 0, 6);
		data_->emitShapeType = uint32_t(shapeType);

		ImGui::DragFloat3("ParticleScale", &data_->scale.x, 0.01f, 0.01f, 300.0f);

		ImGui::SeparatorText("Color");
		ImGui::DragFloat3("ColorMax", &data_->colorMax.x, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat3("ColorMin", &data_->colorMin.x, 0.01f, 0.0f, 1.0f);

		ImGui::SeparatorText("Velocity");
		ImGui::DragFloat3("BaseVelocity", &data_->baseVelocity.x, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("VelocityRandMax", &data_->velocityRandMax, 0.01f, -1.0f, 1.0f);
		ImGui::DragFloat("VelocityRandMin", &data_->velocityRandMin, 0.01f, -1.0f, 1.0f);
		int veloType = int(data_->emitVeloType);
		ImGui::DragInt("EmitVelocityType", &veloType, 0.1f, 0, 5);
		data_->emitVeloType = uint32_t(veloType);
		int moveType = static_cast<int>(data_->isRandomMove);
		const char* items[] = {
			"None",     // 0
			"Gradiate", // 1
			"Curl"      // 2
		};
		if (ImGui::Combo("MoveType", &moveType, items, IM_ARRAYSIZE(items))) {
			data_->isRandomMove = static_cast<uint32_t>(moveType);
		}

		bool isTrailEmit = bool(data_->isTrailEmit);
		ImGui::Checkbox("IsTrailEmit", &isTrailEmit);
		data_->isTrailEmit = uint32_t(isTrailEmit);

		ImGui::Text("DeltaTime1:%f", FPSKeeper::DeltaTime());
		ImGui::Text("DeltaTime2:%f", FPSKeeper::DeltaTimeFrame());
		ImGui::TreePop();

	}
#endif // _DEBUG
}
