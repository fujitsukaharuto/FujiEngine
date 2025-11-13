#include "TextureBasedEmitter.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/SRVManager.h"
#include "ImGuiManager/ImGuiManager.h"

TextureBasedEmitter::TextureBasedEmitter(DXCom* dx) {
	resource_ = dx->CreateBufferResource(dx->GetDevice(), sizeof(EmitterTexture));
	resource_->Map(0, nullptr, reinterpret_cast<void**>(&data_));
	*data_ = {};
	data_->count = 300;
	data_->lifeTime = 60.0f;
	data_->frequency = 0.5f;
	data_->translate = Vector3(0.0f, 0.5f, 0.0f);
	data_->radius = 10.0f;
	data_->emit = 0;
	data_->colorMax = { 1.0f,0.0f,1.0f };
	data_->colorMin = { 0.0f,0.0f,0.0f };
}

void TextureBasedEmitter::InitTextureData(const std::string& fileName) {
	textureForEmit_ = TextureManager::GetInstance()->LoadTexture(fileName);
}

void TextureBasedEmitter::Update(float deltaTime) {
	if (!isEmit_) {
		data_->emit = 0;
		data_->frequencyTime = 0.0f;
		return;
	}

	data_->frequencyTime += deltaTime;
	if (data_->frequency <= data_->frequencyTime) {
		data_->frequencyTime -= data_->frequency;
		data_->emit = 1;
	} else {
		data_->emit = 0;
	}
}

void TextureBasedEmitter::Dispatch(ID3D12GraphicsCommandList* cmd,
	DXCom* dx, [[maybe_unused]] SRVManager* srv, const ParticleCSHandles& shared) {
	if (!isEmit_ || data_->count == 0) return;
	dx->GetPipelineManager()->SetCSPipeline(Pipe::EmitTexParticleCS);
	cmd->SetComputeRootDescriptorTable(0, shared.particleCSUAVHandle);
	cmd->SetComputeRootDescriptorTable(3, shared.freeListIndexUAVHandle);
	cmd->SetComputeRootDescriptorTable(4, shared.freeListUAVHandle);
	cmd->SetComputeRootDescriptorTable(5, textureForEmit_->gpuHandle);
	cmd->SetComputeRootConstantBufferView(1, resource_->GetGPUVirtualAddress());
	cmd->SetComputeRootConstantBufferView(2, shared.perFrameCBV);
	int dispatchCountX = (int(textureForEmit_->meta.width) + 32 - 1) / 32;
	int dispatchCountY = (int(textureForEmit_->meta.height) + 32 - 1) / 32;
	dx->GetCommandList()->Dispatch(dispatchCountX, dispatchCountY, 1);
}

void TextureBasedEmitter::DebugGUI() {
#ifdef _DEBUG
	if (ImGui::TreeNode("ParticleCS Emit Control")) {

		ImGui::Checkbox("IsEmit", &isEmit_);

		int dragCount = int(data_->count);
		ImGui::DragInt("emitCount", &dragCount, 1, 0, 100000);
		data_->count = uint32_t(dragCount);

		ImGui::DragFloat("lifeTime", &data_->lifeTime, 0.1f, 1.0f, 300.0f);
		ImGui::DragFloat("frequency", &data_->frequency, 0.1f, 0.0f, 300.0f);

		ImGui::DragFloat3("translate", &data_->translate.x, 0.1f);

		ImGui::DragFloat("radius", &data_->radius, 0.1f, 0.0f, 300.0f);

		ImGui::SeparatorText("Color");
		ImGui::DragFloat3("colorMax", &data_->colorMax.x, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat3("colorMin", &data_->colorMin.x, 0.01f, 0.0f, 1.0f);

		ImGui::SeparatorText("Velocity");
		ImGui::DragFloat3("baseVelocity", &data_->baseVelocity.x, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("velocityRandMax", &data_->velocityRandMax, 0.01f, -1.0f, 1.0f);
		ImGui::DragFloat("velocityRandMin", &data_->velocityRandMin, 0.01f, -1.0f, 1.0f);

		ImGui::Text("DeltaTime1:%f", FPSKeeper::DeltaTime());
		ImGui::Text("DeltaTime2:%f", FPSKeeper::DeltaTimeFrame());
		ImGui::TreePop();
	}
#endif // _DEBUG
}
