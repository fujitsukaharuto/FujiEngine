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
#include "Engine/DXC/Command/DXCommand.h"
#include "Engine/DXC/Debug/GPUTimer.h"
#include "Engine/Core/Debug/ImGuiManager.h"

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

	gpuTimer_ = std::make_unique<DXC::GPUTimer>();
	gpuTimer_->Initialize(pDxcom->GetDevice(), pDxcom->GetDXCommand()->GetQueue(), kTimer_Count);
}

void ObjectRenderer::Finalize() {
	renderQueue_.clear();
	skinningQueue_.clear();
	if (gpuTimer_) {
		gpuTimer_->Finalize();
		gpuTimer_.reset();
	}

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

	// クリックされたフレームだけ、その画素のオブジェクト番号を拾う
	RenderPicking();

	// パイプラインとライトの結線はルートシグネチャ単位なので、張り替えた後にやり直す
	PreDraw();

	const uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	gpuTimer_->Begin(cList, frameIndex, kTimer_Forward);

	// 登録されたRenderObjectをループで描画
	for (RenderObject* obj : renderQueue_) {
		// 仮想関数なので、実際の型(Object3d または AnimationObject)のDrawが呼ばれる
		obj->Render();
	}

	gpuTimer_->End(cList, frameIndex, kTimer_Forward);
	gpuTimer_->Resolve(cList, frameIndex, kTimer_Forward);

	// 描画が終わったらリストを空にする
	renderQueue_.clear();
}

void ObjectRenderer::BuildRaytracingScene() {
	if (!raytracingScene_ || !raytracingScene_->IsAvailable()) {
		return;
	}

	raytracingScene_->BeginFrame();
	// 反射に映す色。メッシュごとのマテリアルから拾う。当たった物の色を映す設定のときしか使われないので、そのときだけ集める
	if (lightManager_ && lightManager_->GetData().reflectionMode == kReflectionModeObjects) {
		rayLookFrames_ = DXC::kFrameCount_ + 1;
	} else if (rayLookFrames_ > 0) {
		--rayLookFrames_;
	}
	const bool isCollectingLooks = rayLookFrames_ > 0;
	std::vector<RayInstanceData> looks;
	for (RenderObject* obj : renderQueue_) {
		// 加算合成かどうかは Draw() で決まるので、この時点の値はこのフレームのもの
		if (!obj->IsRayOccluder()) {
			continue;
		}

		looks.clear();
		for (size_t i = 0; isCollectingLooks && i < obj->GetMaterialCount(); ++i) {
			Material& material = obj->GetMaterial(i);
			const Math::Vector4 color = material.GetColor();
			RayInstanceData look;
			look.color = { color.x, color.y, color.z };
			look.textureIndex = material.GetTextureSrvIndex();
			look.emissive = material.GetEmissive();
			look.uvScale = material.GetUVScale();
			look.uvOffset = material.GetUVTrans();
			looks.push_back(look);
		}

		if (obj->IsSkinned()) {
			// IsSkinned() を返すのは AnimationModel だけ
			auto* skinned = static_cast<AnimationModel*>(obj);
			raytracingScene_->AddSkinnedInstance(obj, obj->GetModel(),
				skinned->GetSkinnedMeshes(), obj->GetWorldMat(), isCollectingLooks ? &looks : nullptr);
		} else {
			raytracingScene_->AddInstance(obj->GetModel(), obj->GetWorldMat(), isCollectingLooks ? &looks : nullptr);
		}
	}
	raytracingScene_->BuildTlas();
}

void ObjectRenderer::RenderPicking() {
	ModelManager* modelManager = ModelManager::GetInstance();
	if (!modelManager->IsPickingRequested()) {
		return;
	}

	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();

	dxcommon_->GetDXCommand()->SetViewAndScissor(MyWin::kWindowWidth, MyWin::kWindowHeight);
	// 見るのはクリックされた1画素だけ。ビューポートは本描画と同じままシザーで絞る
	const LONG x = static_cast<LONG>(modelManager->GetPickedCoord(0));
	const LONG y = static_cast<LONG>(modelManager->GetPickedCoord(1));
	const D3D12_RECT scissor{ x, y, x + 1, y + 1 };
	cList->RSSetScissorRects(1, &scissor);

	cList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 1回目でいちばん手前の深度を求め、2回目でその深度の持ち主が番号を書く。
	// 1回で済ませると、重なった所でどれが残るかが描画の実行順に左右される
	for (Pipe pipe : { Pipe::PickingDepth, Pipe::PickingID }) {
		dxcommon_->GetPipelineManager()->SetPipeline(pipe);
		modelManager->PickingCommand();

		for (RenderObject* obj : renderQueue_) {
			const D3D12_GPU_VIRTUAL_ADDRESS objIDAddress = obj->GetObjIDGPUVirtualAddress();
			if (!obj->IsPickable() || objIDAddress == 0) {
				continue;
			}
			pPipeManager->SetGraphicsRootCBV(cList, RootName::kObjIDData, objIDAddress);
			// 要るのは変換行列・マテリアル・テクスチャだけで、深度+法線プリパスと同じ
			obj->RenderPrepass();
		}

		// 2回目が読む深度を確定させる
		modelManager->PickingBarrier();
	}
	// シザーとパイプラインは、この後の PreDraw が本描画用へ戻す
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
	// GPUが書き終わっているのは1つ前のフレームぶん
	const uint32_t finishedFrame = (dxcommon_->GetNowFrameCount() + DXC::kFrameCount_ - 1) % DXC::kFrameCount_;
	ImGui::Text("forward  : %.3f ms", gpuTimer_->GetElapsedMS(finishedFrame, kTimer_Forward));

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
}