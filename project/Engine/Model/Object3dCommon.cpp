#include "Object3dCommon.h"
#include "Engine/Model/ModelManager.h"
#include "Engine/DX/DXCom.h"
#include "Light/LightManager.h"

void Object3dCommon::Initialize() {
	dxcommon_ = ModelManager::GetInstance()->ShareDXCom();
}

void Object3dCommon::PreDraw() {
	dxcommon_->GetDXCommand()->SetViewAndscissor();
	dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Normal);
	dxcommon_->GetDXCommand()->GetList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	LightManager::GetInstance()->SetLightCommand(dxcommon_->GetCommandList());
}

void Object3dCommon::PreAnimationDraw() {
	dxcommon_->GetDXCommand()->SetViewAndscissor();
	dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Animation);
	dxcommon_->GetDXCommand()->GetList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	LightManager::GetInstance()->SetLightCommand(dxcommon_->GetCommandList());
}
