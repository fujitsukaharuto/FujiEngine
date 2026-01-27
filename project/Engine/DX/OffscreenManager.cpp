#include "OffscreenManager.h"
#include "DXCom.h"
#include "SRVManager.h"
#include "ImGuiManager/ImGuiManager.h"
#include "MyWindow.h"
#include "PipelineManager.h"
#include "Engine/Camera/CameraManager.h"

using namespace Core;
using namespace Graphics;
using namespace Math;


OffscreenManager::~OffscreenManager() {
	dxcommon_ = nullptr;
}

void OffscreenManager::Initialize(DXCom* pDxcom) {
	dxcommon_ = pDxcom;
}

void OffscreenManager::Update() {
	shockData_.shockTime += 0.025f;
	fireData_.animeTime += 0.025f;
	thunderData_.time += 0.005f;

	thunderData_.time = std::fmodf(thunderData_.time, 1.5f);
	thunderData_.progres = thunderData_.time / 1.5f;

	crtData_.crtTime += 0.025f;

	outlineData_.projectionInverse = Inverse(CameraManager::GetInstance()->GetCamera()->GetProjectionMatrix());
}

void OffscreenManager::DebugGUI() {
#ifdef _DEBUG
	ImGui::Begin("OffScreen Debug");


	bool preIsGrayscale = isGrayscale_;
	bool preIsNonePost = isNonePost_;
	bool preIsShock = isShockWave_;
	bool preIsFire = isFire_;
	bool preIsThunder = isThunder_;

	if (ImGui::TreeNode("OffScreen ShaderPath")) {
		ImGui::Checkbox("PostEffect##checkPost", &isGrayscale_);
		ImGui::Checkbox("None", &isNonePost_);
		ImGui::TreePop();
	}
	if (isGrayscale_ && !(preIsGrayscale)) {
		isNonePost_ = false;
		isShockWave_ = false;
		isFire_ = false;
		isThunder_ = false;
	}
	if (isNonePost_ && !(preIsNonePost)) {
		isGrayscale_ = false;
		isShockWave_ = false;
		isFire_ = false;
		isThunder_ = false;
	}
	if (isShockWave_ && !(preIsShock)) {
		isGrayscale_ = false;
		isNonePost_ = false;
		isFire_ = false;
		isThunder_ = false;
	}
	if (isFire_ && !(preIsFire)) {
		isGrayscale_ = false;
		isNonePost_ = false;
		isShockWave_ = false;
		isThunder_ = false;
	}
	if (isThunder_ && !(preIsThunder)) {
		isGrayscale_ = false;
		isNonePost_ = false;
		isShockWave_ = false;
		isFire_ = false;
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
		ImGui::DragFloat2("center##radialpara1", &radialData_.center.x, 0.01f);
		ImGui::DragFloat("blurWidth##radialpara2", &radialData_.blurWidth, 0.01f);
		ImGui::TreePop();
	}

	ImGui::End();
#endif // _DEBUG

}

void OffscreenManager::EffectListGUI() {
#ifdef _DEBUG
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

		if (ImGui::BeginTable("NowPostEffect##offsc", 1, ImGuiTableFlags_None)) {
			ImGui::TableSetupColumn("NowPostEffect##posteff", ImGuiTableColumnFlags_WidthStretch);
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

	offscreenrtvDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	offscreenrtvDesc_.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	clearColorValue_.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	clearColorValue_.Color[0] = 0.1f;
	clearColorValue_.Color[1] = 0.25f;
	clearColorValue_.Color[2] = 0.5f;
	clearColorValue_.Color[3] = 1.0f;

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		offscreenrt_[i] = dxcommon_->CreateOffscreenTextureResource(dxcommon_->GetDevice(), MyWin::kWindowWidth, MyWin::kWindowHeight, clearColorValue_);

		dxcommon_->GetDevice()->CreateRenderTargetView(offscreenrt_[i].Get(), &offscreenrtvDesc_, dxcommon_->GetRTVHandle(i));
	}


	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		grayCSResource_[i] = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(GrayCS));
		grayCSDataGPU_[i] = nullptr;
		grayCSResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&grayCSDataGPU_[i]));

		shockResource_[i] = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(ShockWaveData));
		shockDataGPU_[i] = nullptr;
		shockResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&shockDataGPU_[i]));

		fireResource_[i] = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(FireElement));
		fireDataGPU_[i] = nullptr;
		fireResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&fireDataGPU_[i]));

		thunderResource_[i] = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(LightningElement));
		thunderDataGPU_[i] = nullptr;
		thunderResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&thunderDataGPU_[i]));

		cRTResource_[i] = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(CRTElemnt));
		crtDataGPU_[i] = nullptr;
		cRTResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&crtDataGPU_[i]));

		outlineResource_[i] = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(OutlineElement));
		outlineDataGPU_[i] = nullptr;
		outlineResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&outlineDataGPU_[i]));

		bloomResource_[i] = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(BloomParams));
		bloomDataGPU_[i] = nullptr;
		bloomResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&bloomDataGPU_[i]));

		radialResource_[i] = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(RadialParams));
		radialDataGPU_[i] = nullptr;
		radialResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&radialDataGPU_[i]));

		vignetteResource_[i] = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(VignetteData));
		vignetteDataGPU_[i] = nullptr;
		vignetteResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&vignetteDataGPU_[i]));
	}

	grayCSData_.gray_ = { 0.2f,0.4f,0.2f };

	shockData_.center = { 0.5f,0.5f,0.0f,0.0f };
	shockData_.shockTime = 0.0f;
	shockData_.radius = 1.0f;
	shockData_.intensity = 0.15f;
	shockData_.padding = 0.0f;

	fireData_.animeTime = 0.0f;
	fireData_.resolution = { 1280.0f, 720.0f };
	fireData_.distortionStrength = 0.18f;
	fireData_.highlightStrength = 0.6f;
	fireData_.detailScale = 7.99f;
	fireData_.rangeMin = { 0.05f,0.3f };
	fireData_.rangeMax = { 0.93f,2.82f };
	fireData_.scale = 1.20f;
	fireData_.speed = 4.01f;
	fireData_.noiseSpeed = -0.12f;
	fireData_.blendStrength = 2.0f;

	thunderData_.time = 0.0f;
	thunderData_.resolution = { 1280.0f, 720.0f };
	thunderData_.mainBranchStrength = 25.0f;
	thunderData_.branchCount = 4.0f;
	thunderData_.branchFade = 20.0f;
	thunderData_.highlightStrength = 15.0f;
	thunderData_.noiseScale = 0.2f;
	thunderData_.noiseSpeed = 5.0f;
	thunderData_.rangeMin = { 0.0f,0.0f };
	thunderData_.rangeMax = { 1.0f,1.0f };
	thunderData_.startPos = { 0.5f,0.3f };
	thunderData_.endPos = { 0.5f,0.8f };
	thunderData_.branchStrngth = 4.0f;
	thunderData_.boltCount = 3.0f;

	crtData_.crtTime = 0.0f;
	crtData_.resolution = { 1280.0f, 720.0f };

	outlineData_.projectionInverse = MakeIdentity4x4();

	bloomData_.bloomIntensity = 1.0f;
	bloomData_.bloomThreshold = 0.75f;

	radialData_.center = Vector2(0.5f, 0.5f);
	radialData_.blurWidth = 0.01f;

	vignetteData_.color_ = { 0.8f,0.0f,0.0f };

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		CopyData(i);
	}

	isGrayscale_ = true;
	isNonePost_ = false;
	isShockWave_ = false;
	isFire_ = false;
	isThunder_ = false;

}

void OffscreenManager::SettingTexture() {
	SettingVertex();

	SRVManager* srvManager = SRVManager::GetInstance();
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		offscreenSRVIndex_[i] = srvManager->Allocate();
		offscreenIndex_[i] = srvManager->Allocate();

		srvManager->CreateTextureSRV(offscreenSRVIndex_[i], offscreenrt_[i].Get(), DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1, false);

		offTextureHandleCPU_[i] = srvManager->GetCPUDescriptorHandle(offscreenSRVIndex_[i]);
		offTextureHandle_[i] = srvManager->GetGPUDescriptorHandle(offscreenSRVIndex_[i]);
		offTextureUAVHandleCPU_[i] = srvManager->GetCPUDescriptorHandle(offscreenIndex_[i]);
		offTextureUAVHandle_[i] = srvManager->GetGPUDescriptorHandle(offscreenIndex_[i]);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		dxcommon_->GetDevice()->CreateUnorderedAccessView(offscreenrt_[i].Get(), nullptr, &uavDesc, offTextureUAVHandleCPU_[i]);
	}


	baseTex_ = TextureManager::GetInstance()->LoadTexture("Gradient02.jpg");
	voronoTex_ = TextureManager::GetInstance()->LoadTexture("T_Noise04.jpg");
	noiseTex_ = TextureManager::GetInstance()->LoadTexture("T_Noise02-300x300.jpg");
	noiseDirTex_ = TextureManager::GetInstance()->LoadTexture("Noise_Dir.jpg");
	noiseDirTex_ = TextureManager::GetInstance()->LoadTexture("worley_Noise.jpg");
	noiseDirTex_ = TextureManager::GetInstance()->LoadTexture("perlin_Noise.png");
	nowTex_ = 2;

	InitializePostEffects();
}

void OffscreenManager::Command() {

	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	CopyData(frameIndex);

	if (isGrayscale_) {

		dxcommon_->TransitionResource(offscreenrt_[frameIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);


		ComPtr<ID3D12Resource> ping = offscreenrt_[frameIndex];
		ComPtr<ID3D12Resource> pong = outputTexture_[frameIndex];
		bool isUsePing = true;


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


		auto finalOutput = isUsePing ? pong : ping;
		auto finalSRVHandle = isUsePing ? outputSRVHandle_[frameIndex] : offTextureHandle_[frameIndex];

		if (validPostEffects_.size() != 0) {
			dxcommon_->TransitionResource(finalOutput.Get(),
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
		} else {
			dxcommon_->TransitionResource(finalOutput.Get(),
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_GENERIC_READ);
		}

		dxcommon_->GetDXCommand()->SetViewAndScissor();
		dxcommon_->GetPipelineManager()->SetPipeline(Pipe::None);

		dxcommon_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexGrayBufferView_);
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(0, finalSRVHandle);
		dxcommon_->GetCommandList()->DrawInstanced(3, 1, 0, 0);


		if (isUsePing) {
			dxcommon_->TransitionResource(ping.Get(),
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_GENERIC_READ);

			dxcommon_->TransitionResource(pong.Get(),
				D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		} else {
			if (validPostEffects_.size() != 0) {
				dxcommon_->TransitionResource(pong.Get(),
					D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			} else {

			}
		}

	} else {
		dxcommon_->TransitionResource(offscreenrt_[frameIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
	}

	if (isNonePost_) {
		dxcommon_->GetDXCommand()->SetViewAndScissor();
		dxcommon_->GetPipelineManager()->SetPipeline(Pipe::None);

		dxcommon_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexGrayBufferView_);
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(0, offTextureHandle_[frameIndex]);
		dxcommon_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
	}

	if (isShockWave_) {
		dxcommon_->GetDXCommand()->SetViewAndScissor();
		dxcommon_->GetPipelineManager()->SetPipeline(Pipe::ShockWave);

		dxcommon_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexGrayBufferView_);
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(0, offTextureHandle_[frameIndex]);
		dxcommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, shockResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		dxcommon_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
	}

	if (isFire_) {
		dxcommon_->GetDXCommand()->SetViewAndScissor();
		dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Fire);

		dxcommon_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexGrayBufferView_);
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(0, offTextureHandle_[frameIndex]);
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(1, baseTex_->gpuHandle);
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, voronoTex_->gpuHandle);
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(3, noiseTex_->gpuHandle);
		dxcommon_->GetCommandList()->SetGraphicsRootConstantBufferView(4, fireResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		dxcommon_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
	}


	if (isThunder_) {
		dxcommon_->GetDXCommand()->SetViewAndScissor();
		dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Thunder);

		dxcommon_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexGrayBufferView_);
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(0, offTextureHandle_[frameIndex]);
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(1, noiseDirTex_->gpuHandle);
		dxcommon_->GetCommandList()->SetGraphicsRootConstantBufferView(2, thunderResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		dxcommon_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
	}

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

void OffscreenManager::SettingVertex() {
	vertexGrayResource_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(GrayscaleVertex) * 3);

	vertexGrayBufferView_.BufferLocation = vertexGrayResource_->GetGPUVirtualAddress();
	vertexGrayBufferView_.SizeInBytes = sizeof(GrayscaleVertex) * 3;
	vertexGrayBufferView_.StrideInBytes = sizeof(GrayscaleVertex);

	grayVertexDate_ = nullptr;
	vertexGrayResource_->Map(0, nullptr, reinterpret_cast<void**>(&grayVertexDate_));

	// でっかい三角形をセット
	grayVertexDate_[0] = { Vector4(-1.0f, -1.0f, 0.0f, 1.0f), Vector2(0.0f, 1.0f) }; // 左下
	grayVertexDate_[1] = { Vector4(-1.0f, 3.0f, 0.0f, 1.0f), Vector2(0.0f, -1.0f) };  // 左上（画面外へ）
	grayVertexDate_[2] = { Vector4(3.0f, -1.0f, 0.0f, 1.0f), Vector2(2.0f, 1.0f) };


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

void OffscreenManager::InitializePostEffects() {
	postEffects_.push_back({
		Pipe::GrayCS,
		PostEffectList::Gray,
		[=](auto* cmd, auto input, auto output) {
			cmd->SetComputeRootDescriptorTable(0, input);
			cmd->SetComputeRootDescriptorTable(1, output);
			cmd->SetComputeRootConstantBufferView(2, grayCSResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		}
		});

	postEffects_.push_back({
		Pipe::CRTCS,
		PostEffectList::CRT,
		[=](auto* cmd, auto input, auto output) {
			cmd->SetComputeRootDescriptorTable(0, input);
			cmd->SetComputeRootDescriptorTable(1, output);
			cmd->SetComputeRootConstantBufferView(2, cRTResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		}
		});

	postEffects_.push_back({
		Pipe::RetroTVCS,
		PostEffectList::RetroTV,
		[=](auto* cmd, auto input, auto output) {
			cmd->SetComputeRootDescriptorTable(0, input);
			cmd->SetComputeRootDescriptorTable(1, output);
			cmd->SetComputeRootConstantBufferView(2, cRTResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		}
		});

	postEffects_.push_back({
		Pipe::GaussCS,
		PostEffectList::Gauss,
		[=](auto* cmd, auto input, [[maybe_unused]] auto output) {
			cmd->SetComputeRootDescriptorTable(0, input);
			cmd->SetComputeRootDescriptorTable(1, output);
		}
		});

	postEffects_.push_back({
		Pipe::BoxFilterCS,
		PostEffectList::BoxFilter,
		[=](auto* cmd, auto input, [[maybe_unused]] auto output) {
			cmd->SetComputeRootDescriptorTable(0, input);
			cmd->SetComputeRootDescriptorTable(1, output);
		}
		});

	postEffects_.push_back({
		Pipe::RadialCS,
		PostEffectList::Radial,
		[=](auto* cmd, auto input, [[maybe_unused]] auto output) {
			cmd->SetComputeRootDescriptorTable(0, input);
			cmd->SetComputeRootDescriptorTable(1, output);
			cmd->SetComputeRootConstantBufferView(2, radialResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		}
		});

	postEffects_.push_back({
		Pipe::VignetteCS,
		PostEffectList::Vignette,
		[=](auto* cmd, auto input, [[maybe_unused]] auto output) {
			cmd->SetComputeRootDescriptorTable(0, input);
			cmd->SetComputeRootDescriptorTable(1, output);
			cmd->SetComputeRootConstantBufferView(2, vignetteResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		}
		});

	postEffects_.push_back({
		Pipe::OutlineCS,
		PostEffectList::Outline,
		[=](auto* cmd, auto input, [[maybe_unused]] auto output) {
			cmd->SetComputeRootDescriptorTable(0, input);
			cmd->SetComputeRootDescriptorTable(2, output);
			cmd->SetComputeRootDescriptorTable(1, dxcommon_->GetDepthTexGPUHandle());
			cmd->SetComputeRootConstantBufferView(3, outlineResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		}
		});

	postEffects_.push_back({
		Pipe::LuminanceOutlineCS,
		PostEffectList::LuminanceOutline,
		[=](auto* cmd, auto input, [[maybe_unused]] auto output) {
			cmd->SetComputeRootDescriptorTable(0, input);
			cmd->SetComputeRootDescriptorTable(1, output);
		}
		});

	postEffects_.push_back({
		Pipe::BloomCS,
		PostEffectList::Bloom,
		[=](auto* cmd, auto input, [[maybe_unused]] auto output) {
			cmd->SetComputeRootDescriptorTable(0, input);
			cmd->SetComputeRootDescriptorTable(1, output);
			cmd->SetComputeRootConstantBufferView(2, bloomResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		}
		});

	postEffects_.push_back({
		Pipe::RandomCS,
		PostEffectList::Random,
		[=](auto* cmd, auto input, [[maybe_unused]] auto output) {
			cmd->SetComputeRootDescriptorTable(0, input);
			cmd->SetComputeRootDescriptorTable(1, output);
			cmd->SetComputeRootConstantBufferView(2, cRTResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
		}
		});

}

void OffscreenManager::CopyData(uint32_t frameIndex) {

	grayCSDataGPU_[frameIndex]->gray_ = grayCSData_.gray_;


	vignetteDataGPU_[frameIndex]->color_ = vignetteData_.color_;


	shockDataGPU_[frameIndex]->center = shockData_.center;
	shockDataGPU_[frameIndex]->shockTime = shockData_.shockTime;
	shockDataGPU_[frameIndex]->radius = shockData_.radius;
	shockDataGPU_[frameIndex]->intensity = shockData_.intensity;


	fireDataGPU_[frameIndex]->animeTime = fireData_.animeTime;
	fireDataGPU_[frameIndex]->resolution = fireData_.resolution;
	fireDataGPU_[frameIndex]->distortionStrength = fireData_.distortionStrength;
	fireDataGPU_[frameIndex]->highlightStrength = fireData_.highlightStrength;
	fireDataGPU_[frameIndex]->detailScale = fireData_.detailScale;
	fireDataGPU_[frameIndex]->rangeMin = fireData_.rangeMin;
	fireDataGPU_[frameIndex]->rangeMax = fireData_.rangeMax;
	fireDataGPU_[frameIndex]->scale = fireData_.scale;
	fireDataGPU_[frameIndex]->speed = fireData_.speed;
	fireDataGPU_[frameIndex]->noiseSpeed = fireData_.noiseSpeed;
	fireDataGPU_[frameIndex]->blendStrength = fireData_.blendStrength;


	crtDataGPU_[frameIndex]->crtTime = crtData_.crtTime;
	crtDataGPU_[frameIndex]->resolution = crtData_.resolution;


	outlineDataGPU_[frameIndex]->projectionInverse = outlineData_.projectionInverse;


	bloomDataGPU_[frameIndex]->bloomThreshold = bloomData_.bloomThreshold;
	bloomDataGPU_[frameIndex]->bloomIntensity = bloomData_.bloomIntensity;


	radialDataGPU_[frameIndex]->center = radialData_.center;
	radialDataGPU_[frameIndex]->blurWidth = radialData_.blurWidth;


	thunderDataGPU_[frameIndex]->startPos = thunderData_.startPos;
	thunderDataGPU_[frameIndex]->endPos = thunderData_.endPos;
	thunderDataGPU_[frameIndex]->rangeMin = thunderData_.rangeMin;
	thunderDataGPU_[frameIndex]->rangeMax = thunderData_.rangeMax;
	thunderDataGPU_[frameIndex]->resolution = thunderData_.resolution;
	thunderDataGPU_[frameIndex]->time = thunderData_.time;
	thunderDataGPU_[frameIndex]->mainBranchStrength = thunderData_.mainBranchStrength;
	thunderDataGPU_[frameIndex]->branchCount = thunderData_.branchCount;
	thunderDataGPU_[frameIndex]->branchFade = thunderData_.branchFade;
	thunderDataGPU_[frameIndex]->highlightStrength = thunderData_.highlightStrength;
	thunderDataGPU_[frameIndex]->noiseScale = thunderData_.noiseScale;
	thunderDataGPU_[frameIndex]->noiseSpeed = thunderData_.noiseSpeed;
	thunderDataGPU_[frameIndex]->branchStrngth = thunderData_.branchStrngth;
	thunderDataGPU_[frameIndex]->boltCount = thunderData_.boltCount;
	thunderDataGPU_[frameIndex]->progres = thunderData_.progres;
}
