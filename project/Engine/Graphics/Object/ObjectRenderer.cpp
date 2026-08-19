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
#include "Engine/Graphics/GBuffer/GBufferPass.h"
#include "Engine/Graphics/Raytracing/RayTracedAOPass.h"
#include "Engine/Graphics/Raytracing/RayTracedShadowPass.h"
#include "Engine/Graphics/IBL/IBLBaker.h"

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

	gbufferPass_ = std::make_unique<GBufferPass>();
	gbufferPass_->Initialize(pDxcom);

	aoPass_ = std::make_unique<RayTracedAOPass>();
	aoPass_->Initialize(pDxcom, pLightManager);

	shadowPass_ = std::make_unique<RayTracedShadowPass>();
	shadowPass_->Initialize(pDxcom, pLightManager);

	iblBaker_ = std::make_unique<IBLBaker>();
	iblBaker_->Initialize(pDxcom);
}

void ObjectRenderer::Finalize() {
	renderQueue_.clear();
	skinningQueue_.clear();

	if (raytracingScene_) {
		raytracingScene_->Finalize();
		raytracingScene_.reset();
	}

	if (gbufferPass_) {
		gbufferPass_->Finalize();
		gbufferPass_.reset();
	}

	if (aoPass_) {
		aoPass_->Finalize();
		aoPass_.reset();
	}

	if (iblBaker_) {
		iblBaker_->Finalize();
		iblBaker_.reset();
	}

	if (shadowPass_) {
		shadowPass_->Finalize();
		shadowPass_.reset();
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
	// 環境マップから IBL を焼く。焼くのは最初の1フレームだけで、以降は素通りする。
	// 描くものが無いフレームでも焼いておきたいので、キューの判定より前に置く
	if (iblBaker_) {
		iblBaker_->EnsureBaked(dxcommon_->GetCommandList());
		iblBaker_->RenderPreview(dxcommon_->GetCommandList());
	}

	// 描画するものがなければ何もしない
	if (renderQueue_.empty()) {
		PreDraw();
		return;
	}

	// 描画中のシェーダがレイを飛ばすので、TLASは描画より先に組む
	BuildRaytracingScene();

	// 画面空間のレイトレが読む深度と法線を先に書き出す。
	// 読み手がいないなら走らせない＝AOもソフトシャドウも切っている間はプリパスの負荷も掛からない
	bool needsGBuffer = false;
	if (lightManager_ != nullptr) {
		const AllLightsData& lights = lightManager_->GetData();
		needsGBuffer = (lights.aoMode == kAOModeScreen) || (lights.shadowMode == kShadowModeSoft);
	}

	if (gbufferPass_) {
		gbufferPass_->SetEnabled(needsGBuffer);
		gbufferPass_->Render(renderQueue_);

		// 走らせるかどうかは各パスが自分で見る。走らないときに蓄積を捨てるのもパス側の仕事
		if (aoPass_) {
			aoPass_->Render(*gbufferPass_);
		}
		if (shadowPass_) {
			shadowPass_->Render(*gbufferPass_);
		}

		if (gbufferPass_->IsRendered()) {
			// プリパスがレンダーターゲットを張り替えているので本描画用へ戻す
			dxcommon_->SetRenderTargets();
		}
	}

	// パイプラインとライトの結線はルートシグネチャ単位なので、張り替えた後にやり直す
	PreDraw();

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

	raytracingScene_->BeginFrame();
	for (RenderObject* obj : renderQueue_) {
		// 加算合成かどうかは Draw() で決まるので、この時点の値はこのフレームのもの
		if (!obj->IsRayOccluder()) {
			continue;
		}

		if (obj->IsSkinned()) {
			// IsSkinned() を返すのは AnimationModel だけ
			auto* skinned = static_cast<AnimationModel*>(obj);
			raytracingScene_->AddSkinnedInstance(obj, obj->GetModel(),
				skinned->GetSkinnedMeshes(), obj->GetWorldMat());
		} else {
			raytracingScene_->AddInstance(obj->GetModel(), obj->GetWorldMat());
		}
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

void ObjectRenderer::DebugGUI() {
#ifdef _DEBUGMODE
	// 各パスは自前で CollapsingHeader を開くので、ここは並べるだけでよい
	if (raytracingScene_) { raytracingScene_->DebugGUI(); }
	if (gbufferPass_) { gbufferPass_->DebugGUI(); }
	if (aoPass_) { aoPass_->DebugGUI(); }
	if (shadowPass_) { shadowPass_->DebugGUI(); }
	if (iblBaker_) { iblBaker_->DebugGUI(); }
#endif // _DEBUGMODE
}

void ObjectRenderer::PreDraw() {
	dxcommon_->GetDXCommand()->SetViewAndScissor(MyWin::kWindowWidth, MyWin::kWindowHeight);
	dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Normal);
	dxcommon_->GetDXCommand()->GetList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	lightManager_->SetLightCommand(dxcommon_->GetCommandList());
	ModelManager::GetInstance()->PickingCommand();
}