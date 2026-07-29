#include "Engine/Graphics/Object/ObjectRenderer.h"
#include "Engine/Graphics/Pipeline/RootNames.h"
// RenderObjectのヘッダーをインクルード（パスはプロジェクト構成に合わせて調整してください）
#include "Engine/Graphics/Object/RenderObject.h"
#include "Engine/Graphics/Object/AnimationModel.h"
#include "Engine/Graphics/SkyBox/SkyBox.h"
#include "Engine/Graphics/Model/ModelManager.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/Graphics/Light/LightManager.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Pipeline/PipeKind.h"
#include "Engine/Graphics/Pipeline/PipelineManager.h"
#include "Engine/Core/App/MyWindow.h"
#include "Engine/Graphics/Raytracing/RaytracingScene.h"

using namespace Core;
using namespace Graphics;
using namespace DXC;

ObjectRenderer::~ObjectRenderer() = default;

ObjectRenderer* ObjectRenderer::GetInstance() {
	static ObjectRenderer instance;
	return &instance;
}

void ObjectRenderer::Initialize(DXCom* pDxcom, LightManager* pLightManager) {
	dxcommon_ = pDxcom;
	lightManager_ = pLightManager;

	raytracingScene_ = std::make_unique<RaytracingScene>();
	raytracingScene_->Initialize(pDxcom);
}

void ObjectRenderer::Finalize() {
	renderQueue_.clear();
	skinningQueue_.clear();

	if (raytracingScene_) {
		raytracingScene_->Finalize();
		raytracingScene_.reset();
	}
}

void ObjectRenderer::Add(RenderObject* object) {
	if (object) {
		renderQueue_.push_back(object);
	}
}

void ObjectRenderer::AddSkinned(AnimationModel* object) {
	if (object) {
		skinningQueue_.push_back(object);
	}
}

void Graphics::ObjectRenderer::SetSkyBox(SkyBox* skyBox) {
	if (skyBox) {
		skyBox_ = skyBox;
	}
}

void ObjectRenderer::Render() {
	PreDraw();

	// 描画するものがなければ何もしない
	if (renderQueue_.empty()) {
		return;
	}

	// ★描画より先にTLASを組む。描画中のシェーダがレイを飛ばす時点で完成している必要がある
	BuildRaytracingScene();

	// 登録されたRenderObjectをループで描画
	for (RenderObject* obj : renderQueue_) {
		// 仮想関数なので、実際の型(Object3d または AnimationObject)のDrawが呼ばれる
		obj->Render();
	}

	// 描画が終わったらリストを空にする
	renderQueue_.clear();
}

void ObjectRenderer::BuildRaytracingScene() {
	if (!raytracingScene_ || !raytracingScene_->IsAvailable()) {
		return;
	}

	// renderQueue_ が「このフレームに描くオブジェクト」そのものなので、
	// 別途インスタンスを集める仕組みを作らずにここから拾える。
	// ★ただし AnimationModel は AddSkinned と Add の両方を呼ぶので renderQueue_ にも入っている。
	// スキンメッシュのBLASはバインドポーズのままで、載せると影だけTポーズになるため除外する
	raytracingScene_->BeginFrame();
	for (RenderObject* obj : renderQueue_) {
		if (obj->IsSkinned()) { continue; }
		raytracingScene_->AddInstance(obj->GetModel(), obj->GetWorldMat());
	}
	raytracingScene_->BuildTlas();
}

void ObjectRenderer::Skinning() {
	// スキニングするものがなければ何もしない
	if (skinningQueue_.empty()) {
		return;
	}

	// 登録されたAnimationModelをループ
	for (AnimationModel* obj : skinningQueue_) {
		obj->CSDispatch();
	}

	// スキニングが終わったらリストを空にする
	skinningQueue_.clear();
}

void ObjectRenderer::RenderSkyBox() {
	if (skyBox_) {
		skyBox_->Render();
	}
	skyBox_ = nullptr;
}

void Graphics::ObjectRenderer::RenderGrid() {
	dxcommon_->GetDXCommand()->SetViewAndScissor(MyWin::kWindowWidth, MyWin::kWindowHeight);
	dxcommon_->GetPipelineManager()->SetPipeline(Pipe::BaseGrid);
	
	PipelineManager::GetInstance()->GetInstance()->SetGraphicsRootCBV(dxcommon_->GetDXCommand()->GetList(), RootName::kCameraInfo, CameraManager::GetInstance()->GetCamera()->GetCameraInfoGPUVirtualAddress());
	// 頂点バッファ・インデックスバッファは「無し」
	dxcommon_->GetDXCommand()->GetList()->IASetVertexBuffers(0, 0, nullptr);
	dxcommon_->GetDXCommand()->GetList()->IASetIndexBuffer(nullptr);
	dxcommon_->GetDXCommand()->GetList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	dxcommon_->GetDXCommand()->GetList()->DrawInstanced(3, 1, 0, 0);
}

void ObjectRenderer::PreDraw() {
	dxcommon_->GetDXCommand()->SetViewAndScissor(MyWin::kWindowWidth, MyWin::kWindowHeight);
	dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Normal);
	dxcommon_->GetDXCommand()->GetList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	lightManager_->SetLightCommand(dxcommon_->GetCommandList());
	ModelManager::GetInstance()->PickingCommand();
}