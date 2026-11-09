#include "Engine/Graphics/PostEffect/OffscreenManager.h"
#include <algorithm>
#include "Engine/Graphics/PostEffect/BloomPass.h"
#include "Engine/Graphics/PostEffect/AutoExposurePass.h"
#include "Engine/Graphics/PostEffect/BufferVisualizer.h"
#include "Engine/Graphics/Pipeline/RootNames.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/DXC/Resource/SRVManager.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/Core/App/MyWindow.h"
#include "Engine/Graphics/Pipeline/PipelineManager.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Object/ObjectRenderer.h"
#include "Engine/Graphics/Light/LightManager.h"
#include "Engine/Graphics/Raytracing/VolumetricLightPass.h"
#include "Engine/Core/Time/FPSKeeper.h"
#include "externals/DirectXTex/d3dx12.h"
#ifdef _DEBUGMODE
#include <ctime>
#include <filesystem>
#include <vector>
#include "externals/DirectXTex/DirectXTex.h"
#include "Engine/Logger/Logger.h"
#endif // _DEBUGMODE

using namespace Core;
using namespace Graphics;
using namespace Math;
using namespace DXC;

namespace {
	// 局所トーンマップの地図の1区画の画素数
	constexpr uint32_t kLocalTonemapBlock = 16;
	constexpr uint32_t kLocalTonemapGroupSize = 8;
}


OffscreenManager::OffscreenManager() = default;

OffscreenManager::~OffscreenManager() {
	if (bloomPass_) {
		bloomPass_->Finalize();
		bloomPass_.reset();
	}
	if (autoExposurePass_) {
		autoExposurePass_->Finalize();
		autoExposurePass_.reset();
	}
	if (bufferVisualizer_) {
		bufferVisualizer_->Finalize();
		bufferVisualizer_.reset();
	}
	dxcommon_ = nullptr;
}

void OffscreenManager::Initialize(DXCom* pDxcom) {
	dxcommon_ = pDxcom;
}

void OffscreenManager::Update() {
#ifdef _DEBUGMODE
	if (isScreenshotPending_) {
		isScreenshotPending_ = false;
		SaveScreenshot();
	}
#endif // _DEBUGMODE
	crtData_.crtTime += FPSKeeper::DeltaTime();

	const Matrix4x4& projection = CameraManager::GetInstance()->GetCamera()->GetProjectionMatrix();
	outlineData_.projectionInverse = Inverse(projection);
	// 透視投影の深度は d = m22 + m32 / z
	dofData_.depthToView = { projection.m[2][2], projection.m[3][2] };
}

void OffscreenManager::DebugGUI() {
#ifdef _DEBUGMODE
	// 別のウィンドウ
	if (bufferVisualizer_) {
		bufferVisualizer_->DebugGUI();
	}

	DebugWindow window{ "OffScreen Debug" };
	if (!window) { return; }

	bool preIsPostEffect = isPostEffect_;
	bool preIsNonePost = isNonePost_;

	if (ImGui::TreeNode("OffScreen ShaderPath")) {
		ImGui::Checkbox("PostEffect##checkPost", &isPostEffect_);
		ImGui::Checkbox("None", &isNonePost_);
		ImGui::TreePop();
	}
	if (isPostEffect_ && !(preIsPostEffect)) {
		isNonePost_ = false;
	}
	if (isNonePost_ && !(preIsNonePost)) {
		isPostEffect_ = false;
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Tonemap")) {
		// 露光や演算子はシーンごとの設定(Hierarchy > Environment)にある。ここは全シーン共通の切り替えだけ
		bool isFXAA = tonemapData_.enableFXAA != 0;
		if (ImGui::Checkbox("FXAA", &isFXAA)) {
			tonemapData_.enableFXAA = isFXAA ? 1u : 0u;
		}
		ImGui::SameLine();
		bool isDither = tonemapData_.enableDither != 0;
		if (ImGui::Checkbox("dither", &isDither)) {
			tonemapData_.enableDither = isDither ? 1u : 0u;
		}
		ImGui::TreePop();
	}

	EffectListGUI();

	if (ImGui::TreeNode("Gray")) {
		ImGui::ColorEdit3("gray", &grayCSData_.gray_.x);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Bloom##settingBloom")) {
		// 強さやしきい値はシーンごとの設定(Hierarchy > Environment)にある
		ImGui::Checkbox("multi scale", &useMultiScaleBloom_);
		if (bloomPass_) {
			bloomPass_->DebugGUI();
		}
		ImGui::TreePop();
	}

	if (autoExposurePass_) {
		autoExposurePass_->DebugGUI();
	}

	if (ImGui::TreeNode("RadialBlur##settingRadialBlur")) {
		ImGui::DragFloat2("center##RadialPara1", &radialData_.center.x, 0.01f);
		ImGui::DragFloat("blurWidth##RadialPara2", &radialData_.blurWidth, 0.01f);
		ImGui::TreePop();
	}

#endif // _DEBUG

}

void OffscreenManager::EffectListGUI() {
#ifdef _DEBUGMODE
	ImGui::SetNextItemOpen(true, ImGuiCond_Once);

	if (ImGui::TreeNode("PostEffectList")) {
		static int currentOffscreenIndex = 0;
		ImGui::Combo("PostEffect##offType", &currentOffscreenIndex,
			"GrayScale\0CRT\0RetroTV\0Gauss\0BoxFilter\0RadialBlur\0Vignette\0Outline\0LuminanceOutline\0Bloom\0Random\0GodRay\0DepthOfField\0");
		if (ImGui::Button("Push##offPush")) {
			validPostEffects_.push_back(postEffects_[currentOffscreenIndex]);
		}ImGui::SameLine();
		if (ImGui::Button("Pop##offPop")) {
			if (validPostEffects_.size() != 0) {
				validPostEffects_.pop_back();
			}
		}

		if (ImGui::BeginTable("NowPostEffect##OffSc", 1, ImGuiTableFlags_None)) {
			ImGui::TableSetupColumn("NowPostEffect##PostEff", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableNextRow();

			ImGui::TableHeadersRow();
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			if (ImGui::BeginListBox("##PostEffectNowList", ImVec2(-FLT_MIN, 100.0f))) {
				for (const auto& effect : validPostEffects_) {
					switch (effect.pipeline) {
					case Pipe::GrayCS:
						ImGui::Text("GrayScale");
						break;
					case Pipe::GaussCS:
						ImGui::Text("Gauss");
						break;
					case Pipe::BoxFilterCS:
						ImGui::Text("BoxFilter");
						break;
					case Pipe::RadialCS:
						ImGui::Text("RadialBlur");
						break;
					case Pipe::VignetteCS:
						ImGui::Text("Vignette");
						break;
					case Pipe::CRTCS:
						ImGui::Text("CRT");
						break;
					case Pipe::RetroTVCS:
						ImGui::Text("RetroTV");
						break;
					case Pipe::OutlineCS:
						ImGui::Text("Outline");
						break;
					case Pipe::LuminanceOutlineCS:
						ImGui::Text("LuminanceOutline");
						break;
					case Pipe::BloomCS:
						ImGui::Text("Bloom");
						break;
					case Pipe::RandomCS:
						ImGui::Text("Random");
						break;
					case Pipe::GodRayCS:
						ImGui::Text("GodRay");
						break;
					case Pipe::DepthOfFieldCS:
						ImGui::Text("DepthOfField");
						break;
					default:
						break;
					}
				}
				ImGui::EndListBox();
			}

			ImGui::EndTable();
		}

		ImGui::TreePop();
	}
#endif // _DEBUG
}

void OffscreenManager::CreateResource() {

	offscreenRTVDesc_.Format = kSceneColorFormat;
	offscreenRTVDesc_.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	clearColorValue_.Format = kSceneColorFormat;
	clearColorValue_.Color[0] = 0.0f;
	clearColorValue_.Color[1] = 0.0f;
	clearColorValue_.Color[2] = 0.0f;
	clearColorValue_.Color[3] = 1.0f;

	clearColorValueForGPU_.Format = kSceneColorFormat;
	clearColorValueForGPU_.Color[0] = 0.0f;
	clearColorValueForGPU_.Color[1] = 0.0f;
	clearColorValueForGPU_.Color[2] = 0.0f;
	clearColorValueForGPU_.Color[3] = 0.0f;

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		offscreenRt_[i] = DXC::Helper::CreateOffscreenTextureResource(dxcommon_->GetDevice(), MyWin::kWindowWidth, MyWin::kWindowHeight, clearColorValue_);
		dxcommon_->GetDevice()->CreateRenderTargetView(offscreenRt_[i].Get(), &offscreenRTVDesc_, dxcommon_->GetOffscreenRTV(i));
	}

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		gpuParticleRt_[i] = DXC::Helper::CreateOffscreenTextureResource(dxcommon_->GetDevice(), MyWin::kWindowWidth / 4, MyWin::kWindowHeight / 4, clearColorValueForGPU_);
		dxcommon_->GetDevice()->CreateRenderTargetView(gpuParticleRt_[i].Get(), &offscreenRTVDesc_, dxcommon_->GetGPUParticleRTV(i));
	}

#ifdef _DEBUGMODE
	// パネルに貼るゲーム画面。バックバッファと同じ形式なので最終パスのパイプラインをそのまま使える
	ID3D12Device* device = dxcommon_->GetDevice();
	gameViewRtvHeap_ = DXC::Helper::CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, DXC::kFrameCount_, false);
	const UINT rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const D3D12_CPU_DESCRIPTOR_HANDLE rtvStart = gameViewRtvHeap_->GetCPUDescriptorHandleForHeapStart();
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		gameViewRt_[i] = DXC::Helper::CreateTexture2D(device, MyWin::kWindowWidth, MyWin::kWindowHeight, DXC::kSwapChainFormat,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr);
		gameViewRtvHandle_[i].ptr = rtvStart.ptr + i * rtvSize;
		device->CreateRenderTargetView(gameViewRt_[i].Get(), nullptr, gameViewRtvHandle_[i]);
	}
#endif // _DEBUGMODE

	InitDataResource();
	InitData();

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		CopyData(i);
	}

	isPostEffect_ = true;
	isNonePost_ = false;

}

void OffscreenManager::SettingTexture() {
	SettingVertex();

	SRVManager* srvManager = SRVManager::GetInstance();
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {// フレーム数分作る
		offscreenSRVIndex_[i] = srvManager->Allocate();
		offscreenIndex_[i] = srvManager->Allocate();

		srvManager->CreateTextureSRV(offscreenSRVIndex_[i], offscreenRt_[i].Get(), kSceneColorFormat, 1, false);

		offTextureHandleCPU_[i] = srvManager->GetCPUDescriptorHandle(offscreenSRVIndex_[i]);
		offTextureHandle_[i] = srvManager->GetGPUDescriptorHandle(offscreenSRVIndex_[i]);
		offTextureUAVHandleCPU_[i] = srvManager->GetCPUDescriptorHandle(offscreenIndex_[i]);
		offTextureUAVHandle_[i] = srvManager->GetGPUDescriptorHandle(offscreenIndex_[i]);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = kSceneColorFormat;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		dxcommon_->GetDevice()->CreateUnorderedAccessView(offscreenRt_[i].Get(), nullptr, &uavDesc, offTextureUAVHandleCPU_[i]);
	}

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {// フレーム数分作る
		gpuParticleSRVIndex_[i] = srvManager->Allocate();

		srvManager->CreateTextureSRV(gpuParticleSRVIndex_[i], gpuParticleRt_[i].Get(), kSceneColorFormat, 1, false);

		gpuParticleHandleCPU_[i] = srvManager->GetCPUDescriptorHandle(gpuParticleSRVIndex_[i]);
		gpuParticleHandle_[i] = srvManager->GetGPUDescriptorHandle(gpuParticleSRVIndex_[i]);
	}

#ifdef _DEBUGMODE
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		const uint32_t gameViewSrvIndex = srvManager->Allocate();
		srvManager->CreateTextureSRV(gameViewSrvIndex, gameViewRt_[i].Get(), DXC::kSwapChainFormat, 1, false);
		gameViewSrvHandle_[i] = srvManager->GetGPUDescriptorHandle(gameViewSrvIndex);
	}
#endif // _DEBUGMODE

	InitializePostEffects();

	bloomPass_ = std::make_unique<BloomPass>();
	bloomPass_->Initialize(dxcommon_);

	autoExposurePass_ = std::make_unique<AutoExposurePass>();
	autoExposurePass_->Initialize(dxcommon_);
	autoExposurePass_->SetSettings(autoExposureSettings_);

	const uint32_t localWidth = (MyWin::kWindowWidth + kLocalTonemapBlock - 1) / kLocalTonemapBlock;
	const uint32_t localHeight = (MyWin::kWindowHeight + kLocalTonemapBlock - 1) / kLocalTonemapBlock;
	for (WorkTexture& map : localTonemapMap_) {
		map.Create(dxcommon_->GetDevice(), DXGI_FORMAT_R16_FLOAT, false, localWidth, localHeight);
	}

#ifdef _DEBUGMODE
	bufferVisualizer_ = std::make_unique<BufferVisualizer>();
	bufferVisualizer_->Initialize(dxcommon_);
#endif // _DEBUGMODE
}

void OffscreenManager::Command() {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	UpdateSunEffects();
	UpdateMotionBlur();
	CopyData(frameIndex);

	if (isPostEffect_) {

		dxcommon_->TransitionResource(offscreenRt_[frameIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		PingPongCommand();

	} else {
		dxcommon_->TransitionResource(offscreenRt_[frameIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
	}

	OtherPipeLineCommand();
}

D3D12_GPU_DESCRIPTOR_HANDLE OffscreenManager::GetGameViewTexture() const {
	return gameViewSrvHandle_[dxcommon_->GetNowFrameCount()];
}

void OffscreenManager::ApplySettings(const PostProcessSettings& settings) {
	bloomData_.bloomThreshold = settings.bloomThreshold;
	bloomData_.bloomIntensity = settings.bloomIntensity;
	if (bloomPass_) {
		bloomPass_->SetShape(settings.bloomKnee, settings.bloomScatter);
	}
	tonemapData_.mode = settings.tonemapMode;
	tonemapData_.exposure = settings.exposure;
	autoExposureSettings_.enabled = settings.autoExposure;
	autoExposureSettings_.minEV = settings.autoExposureMinEV;
	autoExposureSettings_.maxEV = settings.autoExposureMaxEV;
	autoExposureSettings_.speedUp = settings.autoExposureSpeedUp;
	autoExposureSettings_.speedDown = settings.autoExposureSpeedDown;
	if (autoExposurePass_) {
		autoExposurePass_->SetSettings(autoExposureSettings_);
	}
	tonemapData_.saturation = settings.saturation;
	tonemapData_.contrast = settings.contrast;
	tonemapData_.sharpen = settings.sharpen;
	tonemapData_.vignette = settings.vignette;
	tonemapData_.shadowTint = settings.shadowTint;
	tonemapData_.highlightTint = settings.highlightTint;
	tonemapData_.chromaticAberration = settings.chromaticAberration;
	// 正で暖色。見た目の明るさで割り戻す
	const Vector3 balance = {
		1.0f + 0.2f * settings.whiteBalanceTemperature,
		1.0f - 0.15f * settings.whiteBalanceTint,
		1.0f - 0.2f * settings.whiteBalanceTemperature };
	const float balanceLuminance = 0.2126f * balance.x + 0.7152f * balance.y + 0.0722f * balance.z;
	tonemapData_.whiteBalance = balance * (1.0f / (std::max)(balanceLuminance, 1e-3f));

	godRayIntensity_ = settings.godRayIntensity;
	godRayData_.rayLength = settings.godRayLength;
	godRayData_.sunSize = settings.godRaySunSize;

	dofData_.maxBlur = settings.dofBlur;
	dofData_.focusDistance = settings.dofFocusDistance;
	dofData_.focusRange = settings.dofFocusRange;
	dofData_.falloff = settings.dofFalloff;

	lensFlareIntensity_ = settings.lensFlare;
	tonemapData_.purkinje = settings.purkinje;
	tonemapData_.purkinjeLuminance = (std::max)(settings.purkinjeLuminance, 1e-4f);
	tonemapData_.localTonemap = settings.localTonemap;
	tonemapData_.localTonemapDetail = settings.localTonemapDetail;
	tonemapData_.filmGrain = settings.filmGrain;
	tonemapData_.fade = settings.fade;
	motionBlurStrength_ = settings.motionBlur;

	const bool hasBloom = HasPostEffect(PostEffectList::Bloom);
	if (settings.bloomEnabled && !hasBloom) {
		AddPostEffect(PostEffectList::Bloom);
	} else if (!settings.bloomEnabled && hasBloom) {
		PopPostEffect(PostEffectList::Bloom);
	}

	SetSceneEffect(PostEffectList::VolumetricLight, settings.volumetricLight);
	SetSceneEffect(PostEffectList::GodRay, settings.godRayIntensity > 0.0f);
	SetSceneEffect(PostEffectList::DepthOfField, settings.dofBlur > 0.0f);
	SetSceneEffect(PostEffectList::MotionBlur, settings.motionBlur > 0.0f);
	SetSceneEffect(PostEffectList::LensFlare, settings.lensFlare > 0.0f);
}

void OffscreenManager::SetSceneEffect(PostEffectList effect, bool enabled) {
	// 順に: 光の筋 → ゴッドレイ → 被写界深度 → モーションブラー → レンズフレア
	auto rank = [](PostEffectList e) {
		switch (e) {
		case PostEffectList::VolumetricLight: return 0;
		case PostEffectList::GodRay: return 1;
		case PostEffectList::DepthOfField: return 2;
		case PostEffectList::MotionBlur: return 3;
		case PostEffectList::LensFlare: return 4;
		default: return 100;
		}
	};
	const bool hasEffect = HasPostEffect(effect);
	if (enabled && !hasEffect) {
		size_t position = 0;
		while (position < validPostEffects_.size() && rank(validPostEffects_[position].effectName) < rank(effect)) {
			++position;
		}
		validPostEffects_.insert(validPostEffects_.begin() + position, postEffects_[int(effect)]);
	} else if (!enabled && hasEffect) {
		PopPostEffect(effect);
	}
}

bool OffscreenManager::HasPostEffect(PostEffectList effect) const {
	for (const PostEffectPass& pass : validPostEffects_) {
		if (pass.effectName == effect) {
			return true;
		}
	}
	return false;
}

void OffscreenManager::UpdateSunEffects() {
	godRayData_.intensity = 0.0f;
	lensFlareData_.intensity = 0.0f;
	if (godRayIntensity_ <= 0.0f && lensFlareIntensity_ <= 0.0f) {
		return;
	}
	LightManager* lightManager = ObjectRenderer::GetInstance()->GetLightManager();
	if (!lightManager) {
		return;
	}
	const DirectionalLight* light = lightManager->GetDirectionLight(0);
	const Camera* camera = CameraManager::GetInstance()->GetCamera();

	// 太陽は光が進む向きの反対側、無限遠にある。向きだけを変換すればよい(w=0)
	const Vector3 toSun = light->direction * -1.0f;
	const Matrix4x4& view = camera->GetViewMatrix();
	const Matrix4x4& viewProjection = camera->GetViewProjectionMatrix();

	// カメラの正面と太陽のなす角の cos。太陽が視界の横へ外れるにつれて消し、真横より手前で0にする
	const float facing = toSun.x * view.m[0][2] + toSun.y * view.m[1][2] + toSun.z * view.m[2][2];
	const float fade = std::clamp((facing - 0.2f) / 0.4f, 0.0f, 1.0f);
	const float w = toSun.x * viewProjection.m[0][3] + toSun.y * viewProjection.m[1][3] + toSun.z * viewProjection.m[2][3];
	if (fade <= 0.0f || w <= 0.0001f) {
		return;
	}
	const float x = toSun.x * viewProjection.m[0][0] + toSun.y * viewProjection.m[1][0] + toSun.z * viewProjection.m[2][0];
	const float y = toSun.x * viewProjection.m[0][1] + toSun.y * viewProjection.m[1][1] + toSun.z * viewProjection.m[2][1];

	godRayData_.sunUV = { x / w * 0.5f + 0.5f, -y / w * 0.5f + 0.5f };
	godRayData_.color = { light->color.x, light->color.y, light->color.z };
	godRayData_.intensity = godRayIntensity_ * fade;
	lensFlareData_.sunUV = godRayData_.sunUV;
	lensFlareData_.color = godRayData_.color;
	lensFlareData_.intensity = lensFlareIntensity_ * fade;
}

void OffscreenManager::UpdateMotionBlur() {
	motionBlurData_.strength = 0.0f;
	if (motionBlurStrength_ <= 0.0f) {
		// 古いカメラで写し直さないよう前のフレームを忘れる
		hasPreviousViewProjection_ = false;
		return;
	}
	const Matrix4x4& viewProjection = CameraManager::GetInstance()->GetCamera()->GetViewProjectionMatrix();
	if (hasPreviousViewProjection_) {
		motionBlurData_.currentToPrevious = Inverse(viewProjection) * previousViewProjection_;
		motionBlurData_.strength = motionBlurStrength_;
	}
	previousViewProjection_ = viewProjection;
	hasPreviousViewProjection_ = true;
}

void OffscreenManager::PopPostEffect(PostEffectList effect) {
	int popNumber = -1;
	for (int i = 0; i < validPostEffects_.size(); i++) {
		if (validPostEffects_[i].effectName == effect) {
			popNumber = i;
			break;
		}
	}
	if (popNumber == -1)return;
	validPostEffects_.erase(validPostEffects_.begin() + size_t(popNumber));
}

void Graphics::OffscreenManager::BarrierSetForGPURTV() {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	dxcommon_->TransitionResource(gpuParticleRt_[frameIndex].Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void Graphics::OffscreenManager::SynthesisGPURTV() {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	dxcommon_->TransitionResource(gpuParticleRt_[frameIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);

	dxcommon_->GetDXCommand()->SetViewAndScissor(MyWin::kWindowWidth, MyWin::kWindowHeight);
	dxcommon_->GetPipelineManager()->SetPipeline(Pipe::GPUParticleSynthesis); // 加算合成でシーンへ重ねる

	dxcommon_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView_);
	dxcommon_->GetPipelineManager()->SetGraphicsRootDescriptorTable(dxcommon_->GetCommandList(), RootName::kG_InputTexture, gpuParticleHandle_[frameIndex]);
	dxcommon_->GetCommandList()->DrawInstanced(3, 1, 0, 0); // 大きな三角形に描画して負荷を減らす
}

void OffscreenManager::SettingVertex() {
	vertexResource_ = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(GrayscaleVertex) * 3);

	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = sizeof(GrayscaleVertex) * 3;
	vertexBufferView_.StrideInBytes = sizeof(GrayscaleVertex);

	vertexData_ = nullptr;
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

	// でっかい三角形をセット
	vertexData_[0] = { Vector4(-1.0f, -1.0f, 0.0f, 1.0f), Vector2(0.0f, 1.0f) }; // 左下
	vertexData_[1] = { Vector4(-1.0f, 3.0f, 0.0f, 1.0f), Vector2(0.0f, -1.0f) };  // 左上（画面外へ）
	vertexData_[2] = { Vector4(3.0f, -1.0f, 0.0f, 1.0f), Vector2(2.0f, 1.0f) };


	auto device = dxcommon_->GetDevice();

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Width = MyWin::kWindowWidth;
	textureDesc.Height = MyWin::kWindowHeight;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.MipLevels = 1;
	textureDesc.Format = kSceneColorFormat;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = kSceneColorFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = kSceneColorFormat;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	SRVManager* srvManager = SRVManager::GetInstance();

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&outputTexture_[i]));

		// SRV/UAVの作成
		outputSRVIndex_[i] = srvManager->Allocate();
		outputIndex_[i] = srvManager->Allocate();

		outputSRVHandleCPU_[i] = srvManager->GetCPUDescriptorHandle(outputSRVIndex_[i]);
		outputSRVHandle_[i] = srvManager->GetGPUDescriptorHandle(outputSRVIndex_[i]);
		outputUAVHandleCPU_[i] = srvManager->GetCPUDescriptorHandle(outputIndex_[i]);
		outputUAVHandle_[i] = srvManager->GetGPUDescriptorHandle(outputIndex_[i]);

		device->CreateShaderResourceView(outputTexture_[i].Get(), &srvDesc, outputSRVHandleCPU_[i]);
		device->CreateUnorderedAccessView(outputTexture_[i].Get(), nullptr, &uavDesc, outputUAVHandleCPU_[i]);
	}
}

void Graphics::OffscreenManager::InitDataResource() {
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		grayCSResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(GrayCS));
		grayCSDataGPU_[i] = nullptr;
		grayCSResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&grayCSDataGPU_[i]));

		cRTResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(CRTElement));
		crtDataGPU_[i] = nullptr;
		cRTResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&crtDataGPU_[i]));

		outlineResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(OutlineElement));
		outlineDataGPU_[i] = nullptr;
		outlineResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&outlineDataGPU_[i]));

		bloomResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(BloomParams));
		bloomDataGPU_[i] = nullptr;
		bloomResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&bloomDataGPU_[i]));

		radialResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(RadialParams));
		radialDataGPU_[i] = nullptr;
		radialResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&radialDataGPU_[i]));

		vignetteResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(VignetteData));
		vignetteDataGPU_[i] = nullptr;
		vignetteResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&vignetteDataGPU_[i]));

		tonemapResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(TonemapParams));
		tonemapDataGPU_[i] = nullptr;
		tonemapResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&tonemapDataGPU_[i]));

		godRayResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(GodRayParams));
		godRayDataGPU_[i] = nullptr;
		godRayResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&godRayDataGPU_[i]));

		dofResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(DepthOfFieldParams));
		dofDataGPU_[i] = nullptr;
		dofResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&dofDataGPU_[i]));

		lensFlareResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(LensFlareParams));
		lensFlareDataGPU_[i] = nullptr;
		lensFlareResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&lensFlareDataGPU_[i]));

		volumetricResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(VolumetricCompositeParams));
		volumetricDataGPU_[i] = nullptr;
		volumetricResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&volumetricDataGPU_[i]));

		motionBlurResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(MotionBlurParams));
		motionBlurDataGPU_[i] = nullptr;
		motionBlurResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&motionBlurDataGPU_[i]));
	}
}

void Graphics::OffscreenManager::InitData() {
	crtData_.crtTime = 0.0f;
	crtData_.resolution = { MyWin::kWindowWidth, MyWin::kWindowHeight };

	outlineData_.projectionInverse = MakeIdentity4x4();

	vignetteData_.color_ = { 0.8f,0.0f,0.0f };
}

void OffscreenManager::InitializePostEffects() {
	PipelineManager* pPipeManager = dxcommon_->GetPipelineManager();
	postEffects_.push_back({//Grayのセッティング
		Pipe::GrayCS,
		PostEffectList::Gray,
		[=](auto* cmd, auto input, auto output) {
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kInputTexture, input);
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kOutputTexture, output);
			pPipeManager->SetComputeRootCBV(cmd, RootName::kGrayscaleConstantBuffer, grayCSResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		}
		});

	postEffects_.push_back({//CRTのセッティング
		Pipe::CRTCS,
		PostEffectList::CRT,
		[=](auto* cmd, auto input, auto output) {
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kInputTexture, input);
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kOutputTexture, output);
			pPipeManager->SetComputeRootCBV(cmd, RootName::kConstants, cRTResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		}
		});

	postEffects_.push_back({//RetroTVのセッティング
		Pipe::RetroTVCS,
		PostEffectList::RetroTV,
		[=](auto* cmd, auto input, auto output) {
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kInputTexture, input);
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kOutputTexture, output);
			pPipeManager->SetComputeRootCBV(cmd, RootName::kConstants, cRTResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		}
		});

	postEffects_.push_back({//Gaussのセッティング
		Pipe::GaussCS,
		PostEffectList::Gauss,
		[=](auto* cmd, auto input, [[maybe_unused]] auto output) {
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kInputTexture, input);
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kOutputTexture, output);
		}
		});

	postEffects_.push_back({//BoxFilterのセッティング
		Pipe::BoxFilterCS,
		PostEffectList::BoxFilter,
		[=](auto* cmd, auto input, [[maybe_unused]] auto output) {
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kInputTexture, input);
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kOutputTexture, output);
		}
		});

	postEffects_.push_back({//Radialのセッティング
		Pipe::RadialCS,
		PostEffectList::Radial,
		[=](auto* cmd, auto input, [[maybe_unused]] auto output) {
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kInputTexture, input);
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kOutputTexture, output);
			pPipeManager->SetComputeRootCBV(cmd, RootName::kBlurConstantBuffer, radialResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		}
		});

	postEffects_.push_back({//Vignetteのセッティング
		Pipe::VignetteCS,
		PostEffectList::Vignette,
		[=](auto* cmd, auto input, [[maybe_unused]] auto output) {
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kInputTexture, input);
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kOutputTexture, output);
			pPipeManager->SetComputeRootCBV(cmd, RootName::kVignetteConstantBuffer, vignetteResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		}
		});

	postEffects_.push_back({//OutLineのセッティング
		Pipe::OutlineCS,
		PostEffectList::Outline,
		[=](auto* cmd, auto input, [[maybe_unused]] auto output) {
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kInputTexture, input);
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kOutputTexture, output);
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kDepthTexture, dxcommon_->GetDepthTexGPUHandle());
			pPipeManager->SetComputeRootCBV(cmd, RootName::kMaterial, outlineResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		}
		});

	postEffects_.push_back({//LuminanceOutLineのセッティング
		Pipe::LuminanceOutlineCS,
		PostEffectList::LuminanceOutline,
		[=](auto* cmd, auto input, [[maybe_unused]] auto output) {
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kInputTexture, input);
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kOutputTexture, output);
		}
		});

	postEffects_.push_back({//Bloomのセッティング
		Pipe::BloomCS,
		PostEffectList::Bloom,
		[=](auto* cmd, auto input, [[maybe_unused]] auto output) {
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kInputTexture, input);
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kOutputTexture, output);
			pPipeManager->SetComputeRootCBV(cmd, RootName::kBloomParams, bloomResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		}
		});

	postEffects_.push_back({//Randomのセッティング
		Pipe::RandomCS,
		PostEffectList::Random,
		[=](auto* cmd, [[maybe_unused]] auto input, [[maybe_unused]] auto output) {
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kOutputTexture, output);
			pPipeManager->SetComputeRootCBV(cmd, RootName::kConstants, cRTResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		}
		});

	postEffects_.push_back({//GodRayのセッティング
		Pipe::GodRayCS,
		PostEffectList::GodRay,
		[=](auto* cmd, auto input, auto output) {
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kInputTexture, input);
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kOutputTexture, output);
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kDepthTexture, dxcommon_->GetDepthTexGPUHandle());
			pPipeManager->SetComputeRootCBV(cmd, RootName::kGodRayParams, godRayResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		}
		});

	postEffects_.push_back({//DepthOfFieldのセッティング
		Pipe::DepthOfFieldCS,
		PostEffectList::DepthOfField,
		[=](auto* cmd, auto input, auto output) {
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kInputTexture, input);
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kOutputTexture, output);
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kDepthTexture, dxcommon_->GetDepthTexGPUHandle());
			pPipeManager->SetComputeRootCBV(cmd, RootName::kDepthOfFieldParams, dofResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		}
		});

	postEffects_.push_back({//MotionBlurのセッティング
		Pipe::MotionBlurCS,
		PostEffectList::MotionBlur,
		[=](auto* cmd, auto input, auto output) {
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kInputTexture, input);
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kOutputTexture, output);
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kDepthTexture, dxcommon_->GetDepthTexGPUHandle());
			pPipeManager->SetComputeRootCBV(cmd, RootName::kMotionBlurParams, motionBlurResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		}
		});

	postEffects_.push_back({//LensFlareのセッティング
		Pipe::LensFlareCS,
		PostEffectList::LensFlare,
		[=](auto* cmd, auto input, auto output) {
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kInputTexture, input);
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kOutputTexture, output);
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kDepthTexture, dxcommon_->GetDepthTexGPUHandle());
			pPipeManager->SetComputeRootCBV(cmd, RootName::kLensFlareParams, lensFlareResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		}
		});

	postEffects_.push_back({//VolumetricLightのセッティング
		Pipe::VolumetricLightCompositeCS,
		PostEffectList::VolumetricLight,
		[=](auto* cmd, auto input, auto output) {
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kInputTexture, input);
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kOutputTexture, output);
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kDepthTexture, dxcommon_->GetDepthTexGPUHandle());
			// パスが無ければ enabled が0で素通し。表は入力で埋める
			const VolumetricLightPass* volumetric = ObjectRenderer::GetInstance()->GetVolumetricLightPass();
			pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kVolumetricLight, volumetric ? volumetric->GetResultSrvHandle() : input);
			pPipeManager->SetComputeRootCBV(cmd, RootName::kVolumetricCompositeParams, volumetricResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		}
		});

}

void Graphics::OffscreenManager::PingPongCommand() {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();

	Microsoft::WRL::ComPtr<ID3D12Resource> ping = offscreenRt_[frameIndex];
	Microsoft::WRL::ComPtr<ID3D12Resource> pong = outputTexture_[frameIndex];
	bool isUsePing = true;

	// validPostEffectsの中身を回してPingPong形式でRTVになるものを切り替える
	for (int i = 0; i < validPostEffects_.size(); i++) {
		auto inputResource = isUsePing ? ping : pong;
		auto outputResource = isUsePing ? pong : ping;

		auto inputSRVHandle = isUsePing ? offTextureHandle_[frameIndex] : outputSRVHandle_[frameIndex];
		auto outputUAVHandle = isUsePing ? outputUAVHandle_[frameIndex] : offTextureUAVHandle_[frameIndex];

		if (i != 0) {
			dxcommon_->TransitionResource(inputResource.Get(),
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

			dxcommon_->TransitionResource(outputResource.Get(),
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		}

		const Pipe pipeline = validPostEffects_[i].pipeline;
		if (pipeline == Pipe::OutlineCS || pipeline == Pipe::GodRayCS || pipeline == Pipe::DepthOfFieldCS ||
			pipeline == Pipe::MotionBlurCS || pipeline == Pipe::LensFlareCS || pipeline == Pipe::VolumetricLightCompositeCS) {
			// 深度を読む効果
			dxcommon_->TransitionDepthToRead();
			dxcommon_->GetPipelineManager()->SetCSPipeline(validPostEffects_[i].pipeline);
			validPostEffects_[i].setup(dxcommon_->GetCommandList(), inputSRVHandle, outputUAVHandle);
			dxcommon_->GetCommandList()->Dispatch((MyWin::kWindowWidth + 7) / 8, (MyWin::kWindowHeight + 7) / 8, 1);
			dxcommon_->TransitionDepthToWrite();
		} else if (validPostEffects_[i].pipeline == Pipe::BloomCS && useMultiScaleBloom_ && bloomPass_) {
			// 多段のブルームは中で何度もディスパッチするので、1回ぶんの枠には収まらない
			bloomPass_->Render(inputSRVHandle, outputUAVHandle, bloomData_.bloomThreshold, bloomData_.bloomIntensity);
		} else {
			dxcommon_->GetPipelineManager()->SetCSPipeline(validPostEffects_[i].pipeline);
			validPostEffects_[i].setup(dxcommon_->GetCommandList(), inputSRVHandle, outputUAVHandle);
			dxcommon_->GetCommandList()->Dispatch((MyWin::kWindowWidth + 7) / 8, (MyWin::kWindowHeight + 7) / 8, 1);
		}

		isUsePing = !isUsePing;
	}
	isUsePing = !isUsePing;

	auto finalOutput = isUsePing ? pong : ping; // 最後に使ったものはどちらなのか
	auto finalSRVHandle = isUsePing ? outputSRVHandle_[frameIndex] : offTextureHandle_[frameIndex];

	if (validPostEffects_.size() != 0) {
		dxcommon_->TransitionResource(finalOutput.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
	} else {
		dxcommon_->TransitionResource(finalOutput.Get(),
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_GENERIC_READ);
	}

	DrawToScreen(finalSRVHandle);


	if (isUsePing) {// 最後に使ったのがどちらかによってバリアの切り替えを変える
		dxcommon_->TransitionResource(ping.Get(),
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_GENERIC_READ);
		dxcommon_->TransitionResource(pong.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	} else {
		if (validPostEffects_.size() != 0) {
			dxcommon_->TransitionResource(pong.Get(),
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		}
	}
}

void Graphics::OffscreenManager::OtherPipeLineCommand() {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();

	if (isNonePost_) {// 何も描画せず
		DrawToScreen(offTextureHandle_[frameIndex]);
	}
}

void Graphics::OffscreenManager::DrawToScreen(D3D12_GPU_DESCRIPTOR_HANDLE input) {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	ID3D12GraphicsCommandList* commandList = dxcommon_->GetCommandList();

	// トーンマップの直前の画面で明るさを測る
	if (autoExposurePass_) {
		autoExposurePass_->Render(input);
	}
	RenderLocalTonemap(input);

#ifdef _DEBUGMODE
	if (bufferVisualizer_) {
		BufferVisualizer::Sources sources;
		sources.scene = input;
		sources.localTonemap = SRVManager::GetInstance()->GetGPUDescriptorHandle(localTonemapMap_[1].srvIndex);
		sources.hasLocalTonemap = tonemapData_.localTonemap > 0.0f;
		sources.exposure = tonemapData_.exposure;
		sources.depthToView = dofData_.depthToView;
		bufferVisualizer_->Render(sources);
	}
#endif // _DEBUGMODE

#ifdef _DEBUGMODE
	// バックバッファへ戻すのは呼び出し側(DXCom::PostEffect)
	const bool isOnPanel = MyWin::GetGameView().isOnPanel;
	if (isOnPanel) {
		dxcommon_->TransitionResource(gameViewRt_[frameIndex].Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		commandList->OMSetRenderTargets(1, &gameViewRtvHandle_[frameIndex], false, nullptr);
	}
#endif // _DEBUGMODE

	const MyWin::GameView& view = MyWin::GetGameView();
	if (view.isOnPanel) {
		dxcommon_->GetDXCommand()->SetViewAndScissor(MyWin::kWindowWidth, MyWin::kWindowHeight);
	} else {
		// ウィンドウへ直に描く。ウィンドウの比率が違えば余りは黒帯になる
		dxcommon_->GetDXCommand()->SetViewAndScissor(view.x, view.y, view.width, view.height);
	}
	dxcommon_->GetPipelineManager()->SetPipeline(Pipe::None);

	commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
	dxcommon_->GetPipelineManager()->SetGraphicsRootDescriptorTable(commandList, RootName::kG_InputTexture, input);
	dxcommon_->GetPipelineManager()->SetGraphicsRootCBV(commandList, RootName::kTonemapParams, tonemapResource_[frameIndex]->GetGPUVirtualAddress());
	// PS が宣言しているので切っていてもバインドが要る
	if (autoExposurePass_) {
		dxcommon_->GetPipelineManager()->SetGraphicsRootDescriptorTable(commandList, RootName::kAutoExposure, autoExposurePass_->GetExposureSrvHandle());
	}
	dxcommon_->GetPipelineManager()->SetGraphicsRootDescriptorTable(commandList, RootName::kLocalTonemap,
		SRVManager::GetInstance()->GetGPUDescriptorHandle(localTonemapMap_[1].srvIndex));
	commandList->DrawInstanced(3, 1, 0, 0); // 大きな三角形に描画して負荷を減らす

#ifdef _DEBUGMODE
	if (isOnPanel) {
		dxcommon_->TransitionResource(gameViewRt_[frameIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
	if (isScreenshotRequested_) {
		isScreenshotRequested_ = false;
		CaptureScreenshot(isOnPanel);
	}
#endif // _DEBUGMODE
}

void OffscreenManager::RenderLocalTonemap(D3D12_GPU_DESCRIPTOR_HANDLE input) {
	if (tonemapData_.localTonemap <= 0.0f) {
		return;
	}
	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = dxcommon_->GetPipelineManager();
	SRVManager* srvManager = SRVManager::GetInstance();
	const WorkTexture& block = localTonemapMap_[0];
	const WorkTexture& map = localTonemapMap_[1];
	const UINT width = (MyWin::kWindowWidth + kLocalTonemapBlock - 1) / kLocalTonemapBlock;
	const UINT height = (MyWin::kWindowHeight + kLocalTonemapBlock - 1) / kLocalTonemapBlock;
	const UINT groupX = (width + kLocalTonemapGroupSize - 1) / kLocalTonemapGroupSize;
	const UINT groupY = (height + kLocalTonemapGroupSize - 1) / kLocalTonemapGroupSize;

	// 区画ごとの平均
	dxcommon_->TransitionResource(block.resource.Get(), WorkTexture::kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pPipeManager->SetCSPipeline(Pipe::LocalTonemapDownsampleCS);
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kInputTexture, input);
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kOutputTexture, srvManager->GetGPUDescriptorHandle(block.uavIndex));
	cList->Dispatch(groupX, groupY, 1);
	dxcommon_->TransitionResource(block.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, WorkTexture::kReadState);

	// 明るさの近い区画どうしでならす
	dxcommon_->TransitionResource(map.resource.Get(), WorkTexture::kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pPipeManager->SetCSPipeline(Pipe::LocalTonemapBlurCS);
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kInputTexture, srvManager->GetGPUDescriptorHandle(block.srvIndex));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kOutputTexture, srvManager->GetGPUDescriptorHandle(map.uavIndex));
	cList->Dispatch(groupX, groupY, 1);
	dxcommon_->TransitionResource(map.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, WorkTexture::kReadState);
}

void OffscreenManager::CaptureScreenshot([[maybe_unused]] bool isOnPanel) {
#ifdef _DEBUGMODE
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	ID3D12GraphicsCommandList* commandList = dxcommon_->GetCommandList();
	ID3D12Resource* target = gameViewRt_[frameIndex].Get();

	if (!isOnPanel) {
		// パイプラインと入力は直前の描画のまま残っているので、描き先と範囲だけ替えて描き直す。
		// バックバッファへ戻すのは呼び出し側(DXCom::PostEffect)
		dxcommon_->TransitionResource(target, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		commandList->OMSetRenderTargets(1, &gameViewRtvHandle_[frameIndex], false, nullptr);
		dxcommon_->GetDXCommand()->SetViewAndScissor(MyWin::kWindowWidth, MyWin::kWindowHeight);
		commandList->DrawInstanced(3, 1, 0, 0);
		dxcommon_->TransitionResource(target, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	if (!screenshotReadback_) {
		const D3D12_RESOURCE_DESC desc = target->GetDesc();
		UINT64 totalBytes = 0;
		dxcommon_->GetDevice()->GetCopyableFootprints(&desc, 0, 1, 0, &screenshotFootprint_, nullptr, nullptr, &totalBytes);
		screenshotReadback_ = DXC::Helper::CreateReadbackResource(dxcommon_->GetDevice(), static_cast<size_t>(totalBytes));
	}

	dxcommon_->TransitionResource(target, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE);
	const CD3DX12_TEXTURE_COPY_LOCATION destination(screenshotReadback_.Get(), screenshotFootprint_);
	const CD3DX12_TEXTURE_COPY_LOCATION source(target, 0);
	commandList->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
	dxcommon_->TransitionResource(target, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	isScreenshotPending_ = true;
#endif // _DEBUGMODE
}

void OffscreenManager::SaveScreenshot() {
#ifdef _DEBUGMODE
	// 写しを頼んだのは1フレーム前で、GPU がまだ実行中のことがある
	dxcommon_->PerFrameWait();

	const UINT width = screenshotFootprint_.Footprint.Width;
	const UINT height = screenshotFootprint_.Footprint.Height;
	const UINT rowPitch = screenshotFootprint_.Footprint.RowPitch;

	uint8_t* mapped = nullptr;
	const D3D12_RANGE readRange{ 0, static_cast<SIZE_T>(rowPitch) * height };
	if (FAILED(screenshotReadback_->Map(0, &readRange, reinterpret_cast<void**>(&mapped)))) {
		return;
	}
	// アルファには合成の途中の値が残っている。そのまま保存すると透けた画像になるので不透明にする
	std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 4);
	for (UINT y = 0; y < height; ++y) {
		const uint8_t* sourceRow = mapped + screenshotFootprint_.Offset + static_cast<size_t>(y) * rowPitch;
		uint8_t* row = pixels.data() + static_cast<size_t>(y) * width * 4;
		memcpy(row, sourceRow, static_cast<size_t>(width) * 4);
		for (UINT x = 0; x < width; ++x) {
			row[x * 4 + 3] = 255;
		}
	}
	const D3D12_RANGE noWrite{ 0, 0 };
	screenshotReadback_->Unmap(0, &noWrite);

	// 実行ファイルと同じく generated の下へ。git の管理外
	const std::filesystem::path directory = "../generated/screenshots";
	std::error_code error;
	std::filesystem::create_directories(directory, error);

	const std::time_t now = std::time(nullptr);
	std::tm local{};
	localtime_s(&local, &now);
	char name[64] = {};
	std::strftime(name, sizeof(name), "Screenshot_%Y%m%d_%H%M%S.png", &local);
	const std::filesystem::path path = directory / name;

	DirectX::Image image{};
	image.width = width;
	image.height = height;
	image.format = DXC::kSwapChainFormat;
	image.rowPitch = static_cast<size_t>(width) * 4;
	image.slicePitch = image.rowPitch * height;
	image.pixels = pixels.data();
	const HRESULT hr = DirectX::SaveToWICFile(image, DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), path.wstring().c_str());
	Logger::Log(std::string(SUCCEEDED(hr) ? "Screenshot saved: " : "Screenshot failed: ") + path.string() + "\n");
#endif // _DEBUGMODE
}

void OffscreenManager::CopyData(uint32_t frameIndex) {
	grayCSDataGPU_[frameIndex]->gray_ = grayCSData_.gray_;

	vignetteDataGPU_[frameIndex]->color_ = vignetteData_.color_;

	crtDataGPU_[frameIndex]->crtTime = crtData_.crtTime;
	crtDataGPU_[frameIndex]->resolution = crtData_.resolution;

	outlineDataGPU_[frameIndex]->projectionInverse = outlineData_.projectionInverse;

	bloomDataGPU_[frameIndex]->bloomThreshold = bloomData_.bloomThreshold;
	bloomDataGPU_[frameIndex]->bloomIntensity = bloomData_.bloomIntensity;

	radialDataGPU_[frameIndex]->center = radialData_.center;
	radialDataGPU_[frameIndex]->blurWidth = radialData_.blurWidth;

	tonemapDataGPU_[frameIndex]->exposure = tonemapData_.exposure;
	tonemapDataGPU_[frameIndex]->mode = tonemapData_.mode;
	tonemapDataGPU_[frameIndex]->enableFXAA = tonemapData_.enableFXAA;
	tonemapDataGPU_[frameIndex]->enableDither = tonemapData_.enableDither;
	tonemapDataGPU_[frameIndex]->saturation = tonemapData_.saturation;
	tonemapDataGPU_[frameIndex]->contrast = tonemapData_.contrast;
	tonemapDataGPU_[frameIndex]->sharpen = tonemapData_.sharpen;
	tonemapDataGPU_[frameIndex]->vignette = tonemapData_.vignette;
	tonemapDataGPU_[frameIndex]->shadowTint = tonemapData_.shadowTint;
	tonemapDataGPU_[frameIndex]->autoExposure = (autoExposurePass_ && autoExposurePass_->IsEnabled()) ? 1u : 0u;
	tonemapDataGPU_[frameIndex]->highlightTint = tonemapData_.highlightTint;
	tonemapDataGPU_[frameIndex]->chromaticAberration = tonemapData_.chromaticAberration;
	tonemapDataGPU_[frameIndex]->whiteBalance = tonemapData_.whiteBalance;
	tonemapDataGPU_[frameIndex]->purkinje = tonemapData_.purkinje;
	tonemapDataGPU_[frameIndex]->purkinjeLuminance = tonemapData_.purkinjeLuminance;
	tonemapDataGPU_[frameIndex]->localTonemap = tonemapData_.localTonemap;
	tonemapDataGPU_[frameIndex]->localTonemapDetail = tonemapData_.localTonemapDetail;
	tonemapDataGPU_[frameIndex]->filmGrain = tonemapData_.filmGrain;
	tonemapDataGPU_[frameIndex]->fade = tonemapData_.fade;
	// sin の精度が落ちないよう回す
	grainFrame_ = (grainFrame_ + 1) % 1024;
	tonemapDataGPU_[frameIndex]->grainSeed = static_cast<float>(grainFrame_);

	*godRayDataGPU_[frameIndex] = godRayData_;
	*dofDataGPU_[frameIndex] = dofData_;
	*lensFlareDataGPU_[frameIndex] = lensFlareData_;
	*motionBlurDataGPU_[frameIndex] = motionBlurData_;

	VolumetricCompositeParams volumetric{};
	volumetric.depthToView = dofData_.depthToView;
	if (const VolumetricLightPass* pass = ObjectRenderer::GetInstance()->GetVolumetricLightPass()) {
		volumetric.lowSize[0] = pass->GetWidth();
		volumetric.lowSize[1] = pass->GetHeight();
		volumetric.enabled = pass->IsRendered() ? 1u : 0u;
	}
	*volumetricDataGPU_[frameIndex] = volumetric;
}
