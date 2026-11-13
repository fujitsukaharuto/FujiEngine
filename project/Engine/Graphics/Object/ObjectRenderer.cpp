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
#include "Engine/Graphics/SkyBox/SkyAtmosphere.h"
#include "Engine/Graphics/Raytracing/DDGIPass.h"
#include "Engine/Graphics/Raytracing/BakedGIPass.h"
#include "Engine/Graphics/Raytracing/ReflectionProbePass.h"
#include "Engine/Graphics/Particle/RainRenderer.h"
#include "Engine/Graphics/Light/ClusteredLightPass.h"
#include "Engine/Graphics/Raytracing/ScreenProbeGIPass.h"
#include "Engine/Graphics/Raytracing/ScreenReflectionPass.h"
#include "Engine/Graphics/Raytracing/VolumetricLightPass.h"
#include "Engine/Graphics/Object/RefractionCapture.h"
#include "Engine/Graphics/Texture/TextureManager.h"
#include "Engine/DXC/Command/DXCommand.h"
#include "Engine/DXC/Debug/GPUTimer.h"
#include "Engine/Core/Debug/ImGuiManager.h"

#include <algorithm>

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

	skyAtmosphere_ = std::make_unique<SkyAtmosphere>();
	skyAtmosphere_->Initialize(pDxcom);
	pLightManager->SetSkyAtmosphere(skyAtmosphere_.get());

	ddgiPass_ = std::make_unique<DDGIPass>();
	ddgiPass_->Initialize(pDxcom, pLightManager);
	bakedGIPass_ = std::make_unique<BakedGIPass>();
	bakedGIPass_->Initialize(pDxcom, pLightManager);
	reflectionProbePass_ = std::make_unique<ReflectionProbePass>();
	reflectionProbePass_->Initialize(pDxcom, pLightManager);
	rainRenderer_ = std::make_unique<RainRenderer>();
	rainRenderer_->Initialize(pDxcom, pLightManager);
	clusteredLightPass_ = std::make_unique<ClusteredLightPass>();
	clusteredLightPass_->Initialize(pDxcom, pLightManager);

	screenProbeGIPass_ = std::make_unique<ScreenProbeGIPass>();
	screenProbeGIPass_->Initialize(pDxcom, pLightManager);

	screenReflectionPass_ = std::make_unique<ScreenReflectionPass>();
	screenReflectionPass_->Initialize(pDxcom, pLightManager);

	volumetricLightPass_ = std::make_unique<VolumetricLightPass>();
	volumetricLightPass_->Initialize(pDxcom, pLightManager);

	refractionCapture_ = std::make_unique<RefractionCapture>();
	refractionCapture_->Initialize(pDxcom);

	gpuTimer_ = std::make_unique<DXC::GPUTimer>();
	gpuTimer_->Initialize(pDxcom->GetDevice(), pDxcom->GetDXCommand()->GetQueue(), kTimer_Count);
	gpuTimer_->SetName(kTimer_Forward, "  Forward");
	gpuTimer_->SetName(kTimer_AccelBuild, "  Accel build");
	gpuTimer_->SetName(kTimer_Tlas, "    TLAS");
	gpuTimer_->SetName(kTimer_LightCull, "  Light cull");
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

	if (ddgiPass_) {
		ddgiPass_->Finalize();
		ddgiPass_.reset();
	}
	if (bakedGIPass_) {
		bakedGIPass_->Finalize();
		bakedGIPass_.reset();
	}
	if (reflectionProbePass_) {
		reflectionProbePass_->Finalize();
		reflectionProbePass_.reset();
	}
	if (rainRenderer_) {
		rainRenderer_->Finalize();
		rainRenderer_.reset();
	}
	if (clusteredLightPass_) {
		clusteredLightPass_->Finalize();
		clusteredLightPass_.reset();
	}

	if (screenProbeGIPass_) {
		screenProbeGIPass_->Finalize();
		screenProbeGIPass_.reset();
	}

	if (refractionCapture_) {
		refractionCapture_->Finalize();
		refractionCapture_.reset();
	}
	if (volumetricLightPass_) {
		volumetricLightPass_->Finalize();
		volumetricLightPass_.reset();
	}
	if (screenReflectionPass_) {
		screenReflectionPass_->Finalize();
		screenReflectionPass_.reset();
	}

	if (skyAtmosphere_) {
		if (lightManager_) {
			lightManager_->SetSkyAtmosphere(nullptr);
		}
		skyAtmosphere_->Finalize();
		skyAtmosphere_.reset();
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
	// 環境マップから IBL を焼く。焼く元が変わったときだけ
	// 描くものが無いフレームでも焼いておきたいので、キューの判定より前に置く
	if (iblBaker_) {
		// 計算する空のときは空を写したキューブマップから焼く
		const bool isSkyIBL = skyAtmosphere_ && skyAtmosphere_->IsReady() && lightManager_
			&& lightManager_->IsAtmosphereActive() && lightManager_->GetData().ambientMode == kAmbientModeIBL;
		if (isSkyIBL) {
			iblBaker_->SetSource(skyAtmosphere_->GetSkyCubeSrvHandle(), skyAtmosphere_->GetSkyCubeSize(),
				skyAtmosphere_->GetSkyCubeVersion());
		} else {
			iblBaker_->ResetSource();
		}
		iblBaker_->EnsureBaked(dxcommon_->GetCommandList());
		iblBaker_->RenderPreview(dxcommon_->GetCommandList());
	}

	// 描画するものがなければ何もしない
	if (renderQueue_.empty()) {
		PreDraw();
		return;
	}

	// 描画中のシェーダがレイを飛ばすので、TLASは描画より先に組む
	{
		ID3D12GraphicsCommandList* list = dxcommon_->GetCommandList();
		const uint32_t index = dxcommon_->GetNowFrameCount();
		gpuTimer_->Begin(list, index, kTimer_AccelBuild);
		BuildRaytracingScene();
		gpuTimer_->End(list, index, kTimer_AccelBuild);
		gpuTimer_->Resolve(list, index, kTimer_AccelBuild);
	}

	// プローブの間接光
	if (ddgiPass_ && raytracingScene_) {
		ddgiPass_->Render(*raytracingScene_);
	}
	// 焼いた間接光。焼いている途中だけレイを飛ばす
	if (bakedGIPass_ && raytracingScene_) {
		bakedGIPass_->Render(*raytracingScene_);
	}
	// 影なしのライトをクラスタへ振り分ける
	if (clusteredLightPass_) {
		ID3D12GraphicsCommandList* list = dxcommon_->GetCommandList();
		const uint32_t index = dxcommon_->GetNowFrameCount();
		gpuTimer_->Begin(list, index, kTimer_LightCull);
		clusteredLightPass_->Render();
		gpuTimer_->End(list, index, kTimer_LightCull);
		gpuTimer_->Resolve(list, index, kTimer_LightCull);
	}
	// 雨の値を上げ、雨だれの場所を探す
	if (rainRenderer_) {
		rainRenderer_->Prepare(raytracingScene_.get());
	}
	// 反射プローブ。影なしのライトも焼き込むので振り分けの後
	if (reflectionProbePass_ && raytracingScene_ && clusteredLightPass_) {
		reflectionProbePass_->Render(*raytracingScene_, *clusteredLightPass_, rayLookFrames_ > 0);
	}

	// 不透明は手前から描く
	{
		const Math::Vector3 cameraPos = CameraManager::GetInstance()->GetCamera()->GetTranslate();
		const auto firstAfterOpaque = std::stable_partition(renderQueue_.begin(), renderQueue_.end(),
			[](const RenderObject* obj) { return !obj->IsDrawnAfterOpaque(); });
		std::vector<std::pair<float, RenderObject*>> keyed;
		keyed.reserve(static_cast<size_t>(firstAfterOpaque - renderQueue_.begin()));
		for (auto it = renderQueue_.begin(); it != firstAfterOpaque; ++it) {
			const Math::Vector3 d = (*it)->GetWorldPos() - cameraPos;
			keyed.emplace_back(d.x * d.x + d.y * d.y + d.z * d.z, *it);
		}
		std::stable_sort(keyed.begin(), keyed.end(),
			[](const auto& a, const auto& b) { return a.first < b.first; });
		for (size_t i = 0; i < keyed.size(); ++i) {
			renderQueue_[i] = keyed[i].second;
		}
	}

	// 画面空間のレイトレが読む深度と法線を先に書き出す。
	// 読み手がいないなら走らせない＝AOもソフトシャドウも切っている間はプリパスの負荷も掛からない
	bool needsGBuffer = false;
	if (lightManager_ != nullptr) {
		const AllLightsData& lights = lightManager_->GetData();
		needsGBuffer = (lights.aoMode == kAOModeScreen) || (lights.shadowMode == kShadowModeSoft) ||
			(lights.renderFlags & (kRenderFlagScreenGI | kRenderFlagScreenReflection)) != 0 ||
			(volumetricLightPass_ && volumetricLightPass_->IsEnabled());
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
		if (screenProbeGIPass_ && raytracingScene_) {
			screenProbeGIPass_->Render(*gbufferPass_, *raytracingScene_);
		}
		if (screenReflectionPass_ && raytracingScene_) {
			screenReflectionPass_->Render(*gbufferPass_, *raytracingScene_);
		}
		if (volumetricLightPass_ && raytracingScene_ && clusteredLightPass_) {
			volumetricLightPass_->Render(*gbufferPass_, *raytracingScene_, *clusteredLightPass_);
		}

		if (gbufferPass_->IsRendered()) {
			// プリパスがレンダーターゲットを張り替えているので本描画用へ戻す
			dxcommon_->SetRenderTargets();
		}
	}

	// クリックされたフレームだけ、その画素のオブジェクト番号を拾う
	RenderPicking();

	// パイプラインとライトの結線はルートシグネチャ単位なので、張り替えた後にやり直す
	PreDraw(gbufferPass_ && gbufferPass_->IsRendered());

	const uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	gpuTimer_->Begin(cList, frameIndex, kTimer_Forward);

	// 不透明を先に描き、透けるものは後から奥→手前の順に重ねる
	const auto firstAfterOpaque = std::stable_partition(renderQueue_.begin(), renderQueue_.end(),
		[](const RenderObject* obj) { return !obj->IsDrawnAfterOpaque(); });
	const Math::Vector3 cameraPos = CameraManager::GetInstance()->GetCamera()->GetTranslate();
	auto distanceSq = [&cameraPos](const RenderObject* obj) {
		const Math::Vector3 d = obj->GetWorldPos() - cameraPos;
		return d.x * d.x + d.y * d.y + d.z * d.z;
	};
	std::stable_sort(firstAfterOpaque, renderQueue_.end(),
		[&distanceSq](const RenderObject* a, const RenderObject* b) { return distanceSq(a) > distanceSq(b); });

	// 屈折させる半透明の物があるフレームだけ画面を写す
	const bool hasTranslucent = std::any_of(firstAfterOpaque, renderQueue_.end(),
		[](const RenderObject* obj) { return !obj->IsAdd(); });
	const bool isRefracting = refractionCapture_ && refractionCapture_->IsEnabled() && hasTranslucent;
	if (refractionCapture_) {
		refractionCapture_->Prepare(isRefracting);
	}

	// 屈折する物の前に、それまでの半透明ごと写し直す
	constexpr uint32_t kMaxRefractionCaptures = 16;
	uint32_t refractionCaptures = 0;
	bool hasDrawnSinceCapture = false;
	for (auto it = renderQueue_.begin(); it != renderQueue_.end(); ++it) {
		RenderObject* obj = *it;
		if (it == firstAfterOpaque) {
			if (isRefracting) {
				refractionCapture_->Capture();
				refractionCaptures++;
			}
			if (screenReflectionPass_) {
				screenReflectionPass_->CaptureSceneColor();
			}
		} else if (isRefracting && it > firstAfterOpaque && !obj->IsAdd() && hasDrawnSinceCapture &&
			refractionCaptures < kMaxRefractionCaptures) {
			refractionCapture_->Capture();
			refractionCaptures++;
			hasDrawnSinceCapture = false;
		}
		if (it >= firstAfterOpaque) {
			hasDrawnSinceCapture = true;
		}
		// 加算合成は Object3d が自分でパイプラインを張り替えて戻す
		if (it >= firstAfterOpaque && !obj->IsAdd()) {
			dxcommon_->GetPipelineManager()->SetPipeline(Pipe::NormalTranslucent);
		}
		// 仮想関数なので、実際の型(Object3d または AnimationObject)のDrawが呼ばれる
		obj->Render();
	}
	if (firstAfterOpaque != renderQueue_.end()) {
		dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Normal);
	} else if (screenReflectionPass_) {
		screenReflectionPass_->CaptureSceneColor();
	}

	// 雨は加算で重ねる
	if (rainRenderer_ && clusteredLightPass_ && rainRenderer_->GetAmount() > 0.0f) {
		rainRenderer_->Render(*clusteredLightPass_, raytracingScene_.get());
		dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Normal);
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
	// 反射とプローブの間接光に映す色。使う設定のときだけ集める
	if (lightManager_ && (lightManager_->GetData().reflectionMode == kReflectionModeObjects ||
		(lightManager_->GetData().renderFlags & kRenderFlagGI) != 0 || (bakedGIPass_ && bakedGIPass_->IsBaking()) ||
		(reflectionProbePass_ && reflectionProbePass_->NeedsRayLooks()))) {
		rayLookFrames_ = DXC::kFrameCount_ + 1;
	} else if (rayLookFrames_ > 0) {
		--rayLookFrames_;
	}
	const bool isCollectingLooks = rayLookFrames_ > 0;
	// 半透明の物があれば、影のレイがその色と不透明度を見る
	bool hasTranslucentCaster = false;
	std::vector<RayInstanceData> looks;
	for (RenderObject* obj : renderQueue_) {
		// 加算合成かどうかは Draw() で決まるので、この時点の値はこのフレームのもの
		if (!obj->IsRayOccluder()) {
			continue;
		}

		// 抜きのある物は影のレイがアルファを見るので見た目を集める
		bool hasCutout = false;
		for (size_t i = 0; !obj->IsTranslucent() && i < obj->GetMaterialCount(); ++i) {
			hasCutout = hasCutout || obj->GetMaterial(i).IsCutout();
		}
		hasTranslucentCaster = hasTranslucentCaster || obj->IsTranslucent();
		const bool isCollecting = isCollectingLooks || hasCutout || obj->IsTranslucent();

		looks.clear();
		for (size_t i = 0; isCollecting && i < obj->GetMaterialCount(); ++i) {
			Material& material = obj->GetMaterial(i);
			const Math::Vector4 color = material.GetColor();
			RayInstanceData look;
			look.color = { color.x, color.y, color.z };
			look.textureIndex = material.GetTextureSrvIndex();
			look.emissive = material.GetEmissive();
			look.emissiveTextureIndex = material.GetEmissiveTextureSrvIndex();
			look.uvScale = material.GetUVScale();
			look.uvOffset = material.GetUVTrans();
			look.roughness = material.GetEnvironment() > 0.0f ? material.GetRoughness() : 1.0f;
			look.metallic = material.GetMetallic();
			look.opacity = color.w;
			if (material.IsCutout()) {
				const uint32_t alphaRef = static_cast<uint32_t>(std::clamp(material.GetAlphaRef(), 0.0f, 1.0f) * 255.0f + 0.5f);
				look.flags = kRayInstanceCutout | (alphaRef << kRayInstanceAlphaRefShift);
			}
			looks.push_back(look);
		}

		if (obj->IsSkinned()) {
			// IsSkinned() を返すのは AnimationModel だけ
			auto* skinned = static_cast<AnimationModel*>(obj);
			raytracingScene_->AddSkinnedInstance(obj, obj->GetModel(),
				skinned->GetSkinnedMeshes(), obj->GetWorldMat(), isCollecting ? &looks : nullptr, obj->IsTranslucent(), hasCutout);
		} else {
			raytracingScene_->AddInstance(obj->GetModel(), obj->GetWorldMat(), isCollecting ? &looks : nullptr, obj->IsTranslucent(), hasCutout,
				obj->IsStaticLighting());
		}
	}
	if (lightManager_) {
		lightManager_->SetTranslucentShadowThisFrame(hasTranslucentCaster);
	}
	ID3D12GraphicsCommandList* list = dxcommon_->GetCommandList();
	const uint32_t index = dxcommon_->GetNowFrameCount();
	gpuTimer_->Begin(list, index, kTimer_Tlas);
	raytracingScene_->BuildTlas();
	gpuTimer_->End(list, index, kTimer_Tlas);
	gpuTimer_->Resolve(list, index, kTimer_Tlas);
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
	// 計算する空の数表。空を描くより先に作る
	if (skyBox_ && skyAtmosphere_ && lightManager_ && lightManager_->IsAtmosphereActive()) {
		const FogData& fog = lightManager_->GetData().fog;
		skyAtmosphere_->Render(dxcommon_->GetCommandList(), fog.sunDirection, fog.sunColor * fog.skyIntensity,
			fog.cameraPosition.y, fog.cloudCoverage, fog.cloudTime);
	}
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
	if (skyAtmosphere_) { skyAtmosphere_->DebugGUI(); }
	if (ddgiPass_) { ddgiPass_->DebugGUI(); }
	if (bakedGIPass_) { bakedGIPass_->DebugGUI(); }
	if (reflectionProbePass_) { reflectionProbePass_->DebugGUI(); }
	if (rainRenderer_) { rainRenderer_->DebugGUI(); }
	if (clusteredLightPass_) { clusteredLightPass_->DebugGUI(); }
	if (screenProbeGIPass_) { screenProbeGIPass_->DebugGUI(); }
	if (screenReflectionPass_) { screenReflectionPass_->DebugGUI(); }
	if (volumetricLightPass_) { volumetricLightPass_->DebugGUI(); }
#endif // _DEBUGMODE
}

D3D12_GPU_DESCRIPTOR_HANDLE ObjectRenderer::GetEnvironmentSrvHandle() const {
	if (skyAtmosphere_ && skyAtmosphere_->IsReady() && lightManager_ && lightManager_->IsAtmosphereActive()) {
		return skyAtmosphere_->GetSkyCubeSrvHandle();
	}
	// 名前引きは一度でよい
	static Texture* environment = nullptr;
	if (environment == nullptr) {
		environment = TextureManager::GetInstance()->LoadTexture("skyboxTexture.dds");
	}
	return environment ? environment->gpuHandle : D3D12_GPU_DESCRIPTOR_HANDLE{};
}

void ObjectRenderer::PreDraw(bool depthPrepassed) {
	dxcommon_->GetDXCommand()->SetViewAndScissor(MyWin::kWindowWidth, MyWin::kWindowHeight);
	// プリパスの後は深度を書かない。書くと discard のせいで早期Zが効かない
	dxcommon_->GetPipelineManager()->SetPipeline(depthPrepassed ? Pipe::NormalPrepassed : Pipe::Normal);
	dxcommon_->GetDXCommand()->GetList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	lightManager_->SetLightCommand(dxcommon_->GetCommandList());
}