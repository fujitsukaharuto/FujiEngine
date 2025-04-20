#include "Object3dCommon.h"
#include "Engine/DX/DXCom.h"
#include "Light/LightManager.h"

void Object3dCommon::Initialize() {
	dxCommon_ = DXCom::GetInstance();
}

void Object3dCommon::PreDraw() {
	dxCommon_->GetDXCommand()->SetViewAndscissor();
	dxCommon_->GetPipelineManager()->SetPipeline(Pipe::Normal);
	dxCommon_->GetDXCommand()->GetList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	LightManager::GetInstance()->SetLightCommand(DXCom::GetInstance()->GetCommandList());
}

void Object3dCommon::PreAnimationDraw() {
	dxCommon_->GetDXCommand()->SetViewAndscissor();
	dxCommon_->GetPipelineManager()->SetPipeline(Pipe::Animation);
	dxCommon_->GetDXCommand()->GetList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	LightManager::GetInstance()->SetLightCommand(DXCom::GetInstance()->GetCommandList());
}
