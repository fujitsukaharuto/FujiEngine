#include "Engine/DX/OffscreenManager.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/DX12Helper.h"
#include "Engine/DX/SRVManager.h"
#include "Engine/ImGuiManager/ImGuiManager.h"
#include "Engine/WinApp/MyWindow.h"
#include "Engine/GraphicPipeline/PipelineManager.h"
#include "Engine/Camera/CameraManager.h"

using namespace Core;
using namespace Graphics;
using namespace Math;
using namespace DXC;


OffscreenManager::~OffscreenManager() {
	dxcommon_ = nullptr;
}

void OffscreenManager::Initialize(DXCom* pDxcom) {
	dxcommon_ = pDxcom;
}

void OffscreenManager::Update() {
	crtData_.crtTime += FPSKeeper::DeltaTime();

	outlineData_.projectionInverse = Inverse(CameraManager::GetInstance()->GetCamera()->GetProjectionMatrix());
}

void OffscreenManager::DebugGUI() {
#ifdef _DEBUGMODE
	ImGui::Begin("OffScreen Debug");

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

	EffectListGUI();

	if (ImGui::TreeNode("Gray")) {
		ImGui::ColorEdit3("gray", &grayCSData_.gray_.x);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Bloom##settingBloom")) {
		ImGui::DragFloat("bloomIntensity", &bloomData_.bloomIntensity, 0.01f);
		ImGui::DragFloat("bloomThreshold", &bloomData_.bloomThreshold, 0.01f);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("RadialBlur##settingRadialBlur")) {
		ImGui::DragFloat2("center##RadialPara1", &radialData_.center.x, 0.01f);
		ImGui::DragFloat("blurWidth##RadialPara2", &radialData_.blurWidth, 0.01f);
		ImGui::TreePop();
	}

	ImGui::End();
#endif // _DEBUG

}

void OffscreenManager::EffectListGUI() {
#ifdef _DEBUGMODE
	ImGui::SetNextItemOpen(true, ImGuiCond_Once);

	if (ImGui::TreeNode("PostEffectList")) {
		static int currentOffscreenIndex = 0;
		ImGui::Combo("PostEffect##offType", &currentOffscreenIndex,
			"GrayScale\0CRT\0RetroTV\0Gauss\0BoxFilter\0RadialBlur\0Vignette\0Outline\0LuminanceOutline\0Bloom\0Random\0");
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

	offscreenRTVDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	offscreenRTVDesc_.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	clearColorValue_.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	clearColorValue_.Color[0] = 0.0f;
	clearColorValue_.Color[1] = 0.0f;
	clearColorValue_.Color[2] = 0.0f;
	clearColorValue_.Color[3] = 1.0f;

	clearColorValueForGPU_.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	clearColorValueForGPU_.Color[0] = 0.0f;
	clearColorValueForGPU_.Color[1] = 0.0f;
	clearColorValueForGPU_.Color[2] = 0.0f;
	clearColorValueForGPU_.Color[3] = 0.0f;

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		offscreenRt_[i] = DXC::Helper::CreateOffscreenTextureResource(dxcommon_->GetDevice(), MyWin::kWindowWidth, MyWin::kWindowHeight, clearColorValue_);
		dxcommon_->GetDevice()->CreateRenderTargetView(offscreenRt_[i].Get(), &offscreenRTVDesc_, dxcommon_->GetRTVHandle(i + 2));
	}

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		gpuParticleRt_[i] = DXC::Helper::CreateOffscreenTextureResource(dxcommon_->GetDevice(), MyWin::kWindowWidth / 4, MyWin::kWindowHeight / 4, clearColorValueForGPU_);
		dxcommon_->GetDevice()->CreateRenderTargetView(gpuParticleRt_[i].Get(), &offscreenRTVDesc_, dxcommon_->GetRTVHandle(i + 4));
	}

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

		srvManager->CreateTextureSRV(offscreenSRVIndex_[i], offscreenRt_[i].Get(), DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1, false);

		offTextureHandleCPU_[i] = srvManager->GetCPUDescriptorHandle(offscreenSRVIndex_[i]);
		offTextureHandle_[i] = srvManager->GetGPUDescriptorHandle(offscreenSRVIndex_[i]);
		offTextureUAVHandleCPU_[i] = srvManager->GetCPUDescriptorHandle(offscreenIndex_[i]);
		offTextureUAVHandle_[i] = srvManager->GetGPUDescriptorHandle(offscreenIndex_[i]);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		dxcommon_->GetDevice()->CreateUnorderedAccessView(offscreenRt_[i].Get(), nullptr, &uavDesc, offTextureUAVHandleCPU_[i]);
	}

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {// フレーム数分作る
		gpuParticleSRVIndex_[i] = srvManager->Allocate();

		srvManager->CreateTextureSRV(gpuParticleSRVIndex_[i], gpuParticleRt_[i].Get(), DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1, false);

		gpuParticleHandleCPU_[i] = srvManager->GetCPUDescriptorHandle(gpuParticleSRVIndex_[i]);
		gpuParticleHandle_[i] = srvManager->GetGPUDescriptorHandle(gpuParticleSRVIndex_[i]);
	}


	baseTex_ = TextureManager::GetInstance()->LoadTexture("Gradient02.jpg");
	vNoiseTex_ = TextureManager::GetInstance()->LoadTexture("T_Noise04.jpg");
	noiseTex_ = TextureManager::GetInstance()->LoadTexture("T_Noise02-300x300.jpg");
	noiseDirTex_ = TextureManager::GetInstance()->LoadTexture("Noise_Dir.jpg");
	noiseDirTex_ = TextureManager::GetInstance()->LoadTexture("worley_Noise.jpg");
	noiseDirTex_ = TextureManager::GetInstance()->LoadTexture("perlin_Noise.png");

	InitializePostEffects();
}

void OffscreenManager::Command() {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
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
	dxcommon_->GetPipelineManager()->SetGraphicsRootDescriptorTable(dxcommon_->GetCommandList(), "g_InputTexture", gpuParticleHandle_[frameIndex]);
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
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
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
			pPipeManager->SetComputeRootDescriptorTable(cmd, "InputTexture", input);
			pPipeManager->SetComputeRootDescriptorTable(cmd, "outputTexture", output);
			pPipeManager->SetComputeRootCBV(cmd, "GrayscaleConstantBuffer", grayCSResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		}
		});

	postEffects_.push_back({//CRTのセッティング
		Pipe::CRTCS,
		PostEffectList::CRT,
		[=](auto* cmd, auto input, auto output) {
			pPipeManager->SetComputeRootDescriptorTable(cmd, "InputTexture", input);
			pPipeManager->SetComputeRootDescriptorTable(cmd, "outputTexture", output);
			pPipeManager->SetComputeRootCBV(cmd, "Constants", cRTResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		}
		});

	postEffects_.push_back({//RetroTVのセッティング
		Pipe::RetroTVCS,
		PostEffectList::RetroTV,
		[=](auto* cmd, auto input, auto output) {
			pPipeManager->SetComputeRootDescriptorTable(cmd, "InputTexture", input);
			pPipeManager->SetComputeRootDescriptorTable(cmd, "outputTexture", output);
			pPipeManager->SetComputeRootCBV(cmd, "Constants", cRTResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		}
		});

	postEffects_.push_back({//Gaussのセッティング
		Pipe::GaussCS,
		PostEffectList::Gauss,
		[=](auto* cmd, auto input, [[maybe_unused]] auto output) {
			pPipeManager->SetComputeRootDescriptorTable(cmd, "InputTexture", input);
			pPipeManager->SetComputeRootDescriptorTable(cmd, "outputTexture", output);
		}
		});

	postEffects_.push_back({//BoxFilterのセッティング
		Pipe::BoxFilterCS,
		PostEffectList::BoxFilter,
		[=](auto* cmd, auto input, [[maybe_unused]] auto output) {
			pPipeManager->SetComputeRootDescriptorTable(cmd, "InputTexture", input);
			pPipeManager->SetComputeRootDescriptorTable(cmd, "outputTexture", output);
		}
		});

	postEffects_.push_back({//Radialのセッティング
		Pipe::RadialCS,
		PostEffectList::Radial,
		[=](auto* cmd, auto input, [[maybe_unused]] auto output) {
			pPipeManager->SetComputeRootDescriptorTable(cmd, "InputTexture", input);
			pPipeManager->SetComputeRootDescriptorTable(cmd, "outputTexture", output);
			pPipeManager->SetComputeRootCBV(cmd, "BlurConstantBuffer", radialResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		}
		});

	postEffects_.push_back({//Vignetteのセッティング
		Pipe::VignetteCS,
		PostEffectList::Vignette,
		[=](auto* cmd, auto input, [[maybe_unused]] auto output) {
			pPipeManager->SetComputeRootDescriptorTable(cmd, "InputTexture", input);
			pPipeManager->SetComputeRootDescriptorTable(cmd, "outputTexture", output);
			pPipeManager->SetComputeRootCBV(cmd, "VignetteConstantBuffer", vignetteResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		}
		});

	postEffects_.push_back({//OutLineのセッティング
		Pipe::OutlineCS,
		PostEffectList::Outline,
		[=](auto* cmd, auto input, [[maybe_unused]] auto output) {
			pPipeManager->SetComputeRootDescriptorTable(cmd, "InputTexture", input);
			pPipeManager->SetComputeRootDescriptorTable(cmd, "outputTexture", output);
			pPipeManager->SetComputeRootDescriptorTable(cmd, "gDepthTexture", dxcommon_->GetDepthTexGPUHandle());
			pPipeManager->SetComputeRootCBV(cmd, "gMaterial", outlineResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		}
		});

	postEffects_.push_back({//LuminanceOutLineのセッティング
		Pipe::LuminanceOutlineCS,
		PostEffectList::LuminanceOutline,
		[=](auto* cmd, auto input, [[maybe_unused]] auto output) {
			pPipeManager->SetComputeRootDescriptorTable(cmd, "InputTexture", input);
			pPipeManager->SetComputeRootDescriptorTable(cmd, "outputTexture", output);
		}
		});

	postEffects_.push_back({//Bloomのセッティング
		Pipe::BloomCS,
		PostEffectList::Bloom,
		[=](auto* cmd, auto input, [[maybe_unused]] auto output) {
			pPipeManager->SetComputeRootDescriptorTable(cmd, "InputTexture", input);
			pPipeManager->SetComputeRootDescriptorTable(cmd, "outputTexture", output);
			pPipeManager->SetComputeRootCBV(cmd, "BloomParams", bloomResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		}
		});

	postEffects_.push_back({//Randomのセッティング
		Pipe::RandomCS,
		PostEffectList::Random,
		[=](auto* cmd, [[maybe_unused]] auto input, [[maybe_unused]] auto output) {
			pPipeManager->SetComputeRootDescriptorTable(cmd, "outputTexture", output);
			pPipeManager->SetComputeRootCBV(cmd, "Constants", cRTResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		}
		});

}

void Graphics::OffscreenManager::PingPongCommand() {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();

	ComPtr<ID3D12Resource> ping = offscreenRt_[frameIndex];
	ComPtr<ID3D12Resource> pong = outputTexture_[frameIndex];
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

		if (validPostEffects_[i].pipeline == Pipe::OutlineCS) {
			dxcommon_->PreOutline();
			dxcommon_->GetPipelineManager()->SetCSPipeline(validPostEffects_[i].pipeline);
			validPostEffects_[i].setup(dxcommon_->GetCommandList(), inputSRVHandle, outputUAVHandle);
			dxcommon_->GetCommandList()->Dispatch((MyWin::kWindowWidth + 7) / 8, (MyWin::kWindowHeight + 7) / 8, 1);
			dxcommon_->PostOutline();
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

	dxcommon_->GetDXCommand()->SetViewAndScissor(MyWin::kWindowWidth, MyWin::kWindowHeight);
	dxcommon_->GetPipelineManager()->SetPipeline(Pipe::None);

	dxcommon_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView_);
	dxcommon_->GetPipelineManager()->SetGraphicsRootDescriptorTable(dxcommon_->GetCommandList(), "g_InputTexture", finalSRVHandle);
	dxcommon_->GetCommandList()->DrawInstanced(3, 1, 0, 0); // 大きな三角形に描画して負荷を減らす


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
		dxcommon_->GetDXCommand()->SetViewAndScissor(MyWin::kWindowWidth, MyWin::kWindowHeight);
		dxcommon_->GetPipelineManager()->SetPipeline(Pipe::None);

		dxcommon_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView_);
		dxcommon_->GetPipelineManager()->SetGraphicsRootDescriptorTable(dxcommon_->GetCommandList(), "g_InputTexture", offTextureHandle_[frameIndex]);
		dxcommon_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
	}
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
}
