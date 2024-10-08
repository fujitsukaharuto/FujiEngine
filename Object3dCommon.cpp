#include "Object3dCommon.h"

void Object3dCommon::Initialize(Camera* camera) {

	dxCommon_ = DXCom::GetInstance();
	this->defaultCamera_ = camera;

}

void Object3dCommon::PreDraw() {

	dxCommon_->GetDXCommand()->SetViewAndscissor();
	dxCommon_->GetPipelineManager()->SetPipeline(Pipe::Normal);
	dxCommon_->GetDXCommand()->GetList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

}
