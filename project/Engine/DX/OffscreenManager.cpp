#include "OffscreenManager.h"
#include "DXCom.h"
#include "SRVManager.h"
#include "ImGuiManager/ImGuiManager.h"
#include "MyWindow.h"
#include "PipelineManager.h"
#include "Engine/Camera/CameraManager.h"

OffscreenManager::~OffscreenManager() {
	dxcommon_ = nullptr;
}

void OffscreenManager::Initialize(DXCom* pDxcom) {
	dxcommon_ = pDxcom;
}

void OffscreenManager::Update() {
	shockData_->shockTime += 0.025f;
	fireData_->animeTime += 0.025f;
	thunderData_->time += 0.005f;

	thunderData_->time = std::fmodf(thunderData_->time, 1.5f);
	thunderData_->progres = thunderData_->time / 1.5f;

	crtData_->crtTime += 0.025f;

	outlineData_->projectionInverse = Inverse(CameraManager::GetInstance()->GetCamera()->GetProjectionMatrix());
}

void OffscreenManager::DebugGUI() {
#ifdef _DEBUG
	ImGui::Begin("OffScreen Debug");


	bool preIsGrayscale = isGrayscale_;
	bool preIsNonePost = isNonePost_;
	bool preIsMetaBall = isMetaBall_;
	bool preIsGaussian = isGaussian_;
	bool preIsShock = isShockWave_;
	bool preIsFire = isFire_;
	bool preIsThunder = isThunder_;
	bool preIsCRT = isCRT_;
	bool preIsOutline = isOutline_;

	if (ImGui::TreeNode("OffScreen ShaderPath")) {
		ImGui::Checkbox("Gray", &isGrayscale_);
		ImGui::Checkbox("None", &isNonePost_);
		ImGui::Checkbox("Meta", &isMetaBall_);
		ImGui::Checkbox("Blur", &isGaussian_);
		ImGui::Checkbox("shock", &isShockWave_);
		ImGui::Checkbox("fire", &isFire_);
		ImGui::Checkbox("thunder", &isThunder_);
		ImGui::Checkbox("crt", &isCRT_);
		ImGui::Checkbox("outline", &isOutline_);
		ImGui::TreePop();
	}
	if (isGrayscale_ && !(preIsGrayscale)) {
		isNonePost_ = false;
		isMetaBall_ = false;
		isGaussian_ = false;
		isShockWave_ = false;
		isFire_ = false;
		isThunder_ = false;
		isCRT_ = false;
		isOutline_ = false;
	}
	if (isNonePost_ && !(preIsNonePost)) {
		isGrayscale_ = false;
		isMetaBall_ = false;
		isGaussian_ = false;
		isShockWave_ = false;
		isFire_ = false;
		isThunder_ = false;
		isCRT_ = false;
		isOutline_ = false;
	}
	if (isMetaBall_ && !(preIsMetaBall)) {
		isGrayscale_ = false;
		isNonePost_ = false;
		isGaussian_ = false;
		isShockWave_ = false;
		isFire_ = false;
		isThunder_ = false;
		isCRT_ = false;
		isOutline_ = false;
	}
	if (isGaussian_ && !(preIsGaussian)) {
		isGrayscale_ = false;
		isNonePost_ = false;
		isMetaBall_ = false;
		isShockWave_ = false;
		isFire_ = false;
		isThunder_ = false;
		isCRT_ = false;
		isOutline_ = false;
	}
	if (isShockWave_ && !(preIsShock)) {
		isGrayscale_ = false;
		isNonePost_ = false;
		isMetaBall_ = false;
		isGaussian_ = false;
		isFire_ = false;
		isThunder_ = false;
		isCRT_ = false;
		isOutline_ = false;
	}
	if (isFire_ && !(preIsFire)) {
		isGrayscale_ = false;
		isNonePost_ = false;
		isMetaBall_ = false;
		isGaussian_ = false;
		isShockWave_ = false;
		isThunder_ = false;
		isCRT_ = false;
		isOutline_ = false;
	}
	if (isThunder_ && !(preIsThunder)) {
		isGrayscale_ = false;
		isNonePost_ = false;
		isMetaBall_ = false;
		isGaussian_ = false;
		isShockWave_ = false;
		isFire_ = false;
		isCRT_ = false;
		isOutline_ = false;
	}
	if (isCRT_ && !(preIsCRT)) {
		isGrayscale_ = false;
		isNonePost_ = false;
		isMetaBall_ = false;
		isGaussian_ = false;
		isShockWave_ = false;
		isFire_ = false;
		isThunder_ = false;
		isOutline_ = false;
	}
	if (isOutline_ && !(preIsOutline)) {
		isGrayscale_ = false;
		isNonePost_ = false;
		isMetaBall_ = false;
		isGaussian_ = false;
		isShockWave_ = false;
		isFire_ = false;
		isThunder_ = false;
		isCRT_ = false;
	}



	if (ImGui::Button("shock")) {
		shockData_->shockTime = 0.0f;
	}

	if (ImGui::TreeNode("Gray")) {
		ImGui::ColorEdit3("gray", &grayCSData_->gray_.x);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("FireData")) {
		ImGui::DragFloat("animeTime", &fireData_->animeTime, 0.1f, 0.0f, 60.0f);
		ImGui::DragFloat2("resolution", &fireData_->resolution.x);
		ImGui::DragFloat("distortionStrength", &fireData_->distortionStrength, 0.01f);
		ImGui::DragFloat("highlightStrength", &fireData_->highlightStrength, 0.01f);
		ImGui::DragFloat("detailScale", &fireData_->detailScale, 0.01f);
		ImGui::DragFloat2("rangeMin", &fireData_->rangeMin.x, 0.01f);
		ImGui::DragFloat2("rangeMax", &fireData_->rangeMax.x, 0.01f);
		ImGui::DragFloat("scale", &fireData_->scale, 0.01f);
		ImGui::DragFloat("speed", &fireData_->speed, 0.01f);
		ImGui::DragFloat("noiseSpeed", &fireData_->noiseSpeed, 0.01f);
		ImGui::DragFloat("blend", &fireData_->blendStrength, 0.01f);
		ImGui::TreePop();
	}


	if (ImGui::TreeNode("ThunderData")) {
		ImGui::DragFloat2("startPos", &thunderData_->startPos.x, 0.01f);
		ImGui::DragFloat2("endPos", &thunderData_->endPos.x, 0.01f);
		ImGui::DragFloat("time", &thunderData_->time, 0.1f, 0.0f, 60.0f);
		ImGui::DragFloat2("resolution", &thunderData_->resolution.x);
		ImGui::DragFloat("mainBranchStrength", &thunderData_->mainBranchStrength, 0.1f);
		ImGui::DragFloat("branchStrength", &thunderData_->branchStrngth, 0.1f);
		ImGui::DragFloat("branchCount", &thunderData_->branchCount, 1);
		ImGui::DragFloat("branchFade", &thunderData_->branchFade, 0.1f);
		ImGui::DragFloat("highlightStrength", &thunderData_->highlightStrength, 0.1f);
		ImGui::DragFloat("noiseScale", &thunderData_->noiseScale, 0.01f);
		ImGui::DragFloat("noiseSpeed", &thunderData_->noiseSpeed, 0.1f);
		ImGui::DragFloat2("rangeMin", &thunderData_->rangeMin.x, 0.01f);
		ImGui::DragFloat2("rangeMax", &thunderData_->rangeMax.x, 0.01f);
		ImGui::DragFloat("boltCount", &thunderData_->boltCount, 0.1f);
		ImGui::DragFloat("progres", &thunderData_->progres, 0.1f, 0.0f, 1.0f);

		int tex = nowTex;
		ImGui::Combo("sizeType##type", &tex, "Noise_Dir.jpg\0worley_Noise.jpg\0perlin_Noise.png\0");
		if (tex != nowTex) {
			if (tex == 0) {
				noiseDirTex_ = TextureManager::GetInstance()->LoadTexture("Noise_Dir.jpg");
				nowTex = tex;
			}
			if (tex == 1) {
				noiseDirTex_ = TextureManager::GetInstance()->LoadTexture("worley_Noise.jpg");
				nowTex = tex;
			}
			if (tex == 2) {
				noiseDirTex_ = TextureManager::GetInstance()->LoadTexture("perlin_Noise.png");
				nowTex = tex;
			}
		}


		ImGui::TreePop();
	}

	ImGui::End();
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

	offscreenrt_ = dxcommon_->CreateOffscreenTextureResource(dxcommon_->GetDevice(), MyWin::kWindowWidth, MyWin::kWindowHeight, clearColorValue_);

	dxcommon_->GetDevice()->CreateRenderTargetView(offscreenrt_.Get(), &offscreenrtvDesc_, dxcommon_->GetRTVHandle());


	grayCSResource_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(GrayCS));
	grayCSData_ = nullptr;
	grayCSResource_->Map(0, nullptr, reinterpret_cast<void**>(&grayCSData_));
	grayCSData_->gray_ = { 0.2f,0.4f,0.2f };


	shockResource_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(ShockWaveData));
	shockData_ = nullptr;
	shockResource_->Map(0, nullptr, reinterpret_cast<void**>(&shockData_));
	shockData_->center = { 0.5f,0.5f,0.0f,0.0f };
	shockData_->shockTime = 0.0f;
	shockData_->radius = 1.0f;
	shockData_->intensity = 0.15f;
	shockData_->padding = 0.0f;


	fireResource_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(FireElement));
	fireData_ = nullptr;
	fireResource_->Map(0, nullptr, reinterpret_cast<void**>(&fireData_));
	fireData_->animeTime = 0.0f;
	fireData_->resolution = { 1280.0f, 720.0f };
	fireData_->distortionStrength = 0.18f;
	fireData_->highlightStrength = 0.6f;
	fireData_->detailScale = 7.99f;
	fireData_->rangeMin = { 0.05f,0.3f };
	fireData_->rangeMax = { 0.93f,2.82f };
	fireData_->scale = 1.20f;
	fireData_->speed = 4.01f;
	fireData_->noiseSpeed = -0.12f;
	fireData_->blendStrength = 2.0f;


	thunderResource_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(LightningElement));
	thunderData_ = nullptr;
	thunderResource_->Map(0, nullptr, reinterpret_cast<void**>(&thunderData_));
	thunderData_->time = 0.0f;
	thunderData_->resolution = { 1280.0f, 720.0f };
	thunderData_->mainBranchStrength = 25.0f;
	thunderData_->branchCount = 4.0f;
	thunderData_->branchFade = 20.0f;
	thunderData_->highlightStrength = 15.0f;
	thunderData_->noiseScale = 0.2f;
	thunderData_->noiseSpeed = 5.0f;
	thunderData_->rangeMin = { 0.0f,0.0f };
	thunderData_->rangeMax = { 1.0f,1.0f };
	thunderData_->startPos = { 0.5f,0.3f };
	thunderData_->endPos = { 0.5f,0.8f };
	thunderData_->branchStrngth = 4.0f;
	thunderData_->boltCount = 3.0f;


	cRTResource_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(CRTElemnt));
	crtData_ = nullptr;
	cRTResource_->Map(0, nullptr, reinterpret_cast<void**>(&crtData_));
	crtData_->crtTime = 0.0f;
	crtData_->resolution = { 1280.0f, 720.0f };

	outlineResource_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(OutlineElement));
	outlineData_ = nullptr;
	outlineResource_->Map(0, nullptr, reinterpret_cast<void**>(&outlineData_));
	outlineData_->projectionInverse = MakeIdentity4x4();

	isGrayscale_ = false;
	isNonePost_ = true;
	isMetaBall_ = false;
	isGaussian_ = false;
	isShockWave_ = false;
	isFire_ = false;
	isThunder_ = false;
	isCRT_ = false;
	isOutline_ = false;

}

void OffscreenManager::SettingTexture() {
	SettingVertex();

	SRVManager* srvManager = SRVManager::GetInstance();
	offscreenSRVIndex_ = srvManager->Allocate();
	offscreenIndex_ = srvManager->Allocate();

	srvManager->CreateTextureSRV(offscreenSRVIndex_, offscreenrt_.Get(), DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1, false);

	offTextureHandleCPU_ = srvManager->GetCPUDescriptorHandle(offscreenSRVIndex_);
	offTextureHandle_ = srvManager->GetGPUDescriptorHandle(offscreenSRVIndex_);
	offTextureUAVHandleCPU_ = srvManager->GetCPUDescriptorHandle(offscreenIndex_);
	offTextureUAVHandle_ = srvManager->GetGPUDescriptorHandle(offscreenIndex_);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	dxcommon_->GetDevice()->CreateUnorderedAccessView(offscreenrt_.Get(), nullptr, &uavDesc, offTextureUAVHandleCPU_);


	baseTex_ = TextureManager::GetInstance()->LoadTexture("Gradient02.jpg");
	voronoTex_ = TextureManager::GetInstance()->LoadTexture("T_Noise04.jpg");
	noiseTex_ = TextureManager::GetInstance()->LoadTexture("T_Noise02-300x300.jpg");
	noiseDirTex_ = TextureManager::GetInstance()->LoadTexture("Noise_Dir.jpg");
	noiseDirTex_ = TextureManager::GetInstance()->LoadTexture("worley_Noise.jpg");
	noiseDirTex_ = TextureManager::GetInstance()->LoadTexture("perlin_Noise.png");
	nowTex = 2;
}

void OffscreenManager::Command() {

	if (isGrayscale_) {


		dxcommon_->TransitionResource(offscreenrt_.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);


		ComPtr<ID3D12Resource> ping = offscreenrt_;
		ComPtr<ID3D12Resource> pong = outputTexture_;
		bool isUsePing = true;


		for (int i = 0; i < 2; i++) {
			auto inputResource = isUsePing ? ping : pong;
			auto outputResource = isUsePing ? pong : ping;

			auto inputSRVHandle = isUsePing ? offTextureHandle_ : outputSRVHandle_;
			auto outputUAVHandle = isUsePing ? outputUAVHandle_ : offTextureUAVHandle_;


			if (i != 0) {
				dxcommon_->TransitionResource(inputResource.Get(),
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

				dxcommon_->TransitionResource(outputResource.Get(),
					D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			}


			if (i == 0) {
				dxcommon_->GetPipelineManager()->SetCSPipeline(Pipe::GrayCS);
				// 3. SRVとUAVをセット
				dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(0, inputSRVHandle);   // 入力テクスチャ
				dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(1, outputUAVHandle);  // 出力テクスチャ
				// 4. 定数バッファ(CBV)をバインド
				dxcommon_->GetCommandList()->SetComputeRootConstantBufferView(2, grayCSResource_->GetGPUVirtualAddress());    // b0 レジスタ
				// 5. コンピュートシェーダーを実行 (Dispatch)
				dxcommon_->GetCommandList()->Dispatch(
					(MyWin::kWindowWidth + 7) / 8,  // スレッドグループ数 (8x8x1の場合)
					(MyWin::kWindowHeight + 7) / 8,
					1
				);
			}
			if (i == 1) {
				dxcommon_->GetPipelineManager()->SetCSPipeline(Pipe::CRTCS);
				// 3. SRVとUAVをセット
				dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(0, inputSRVHandle);   // 入力テクスチャ
				dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(1, outputUAVHandle);  // 出力テクスチャ
				// 4. 定数バッファ(CBV)をバインド
				dxcommon_->GetCommandList()->SetComputeRootConstantBufferView(2, cRTResource_->GetGPUVirtualAddress());    // b0 レジスタ
				// 5. コンピュートシェーダーを実行 (Dispatch)
				dxcommon_->GetCommandList()->Dispatch(
					(MyWin::kWindowWidth + 7) / 8,  // スレッドグループ数 (8x8x1の場合)
					(MyWin::kWindowHeight + 7) / 8,
					1
				);
			}

			isUsePing = !isUsePing;
		}
		isUsePing = !isUsePing;


		auto finalOutput = isUsePing ? pong : ping;
		auto finalSRVHandle = isUsePing ? outputSRVHandle_ : offTextureHandle_;

		dxcommon_->TransitionResource(finalOutput.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);

		dxcommon_->GetDXCommand()->SetViewAndscissor();
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
			dxcommon_->TransitionResource(pong.Get(),
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			/*dxcommon_->TransitionResource(ping.Get(),
				D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);*/
		}

	} else {
		dxcommon_->TransitionResource(offscreenrt_.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
	}


	if (isGaussian_) {
		dxcommon_->GetDXCommand()->SetViewAndscissor();
		dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Gauss);

		dxcommon_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexGrayBufferView_);
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(0, offTextureHandle_);
		dxcommon_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
	}

	if (isNonePost_ || isMetaBall_) {
		dxcommon_->GetDXCommand()->SetViewAndscissor();
		dxcommon_->GetPipelineManager()->SetPipeline(Pipe::None);

		dxcommon_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexGrayBufferView_);
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(0, offTextureHandle_);
		dxcommon_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
	}

	if (isShockWave_) {
		dxcommon_->GetDXCommand()->SetViewAndscissor();
		dxcommon_->GetPipelineManager()->SetPipeline(Pipe::ShockWave);

		dxcommon_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexGrayBufferView_);
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(0, offTextureHandle_);
		dxcommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, shockResource_->GetGPUVirtualAddress());
		dxcommon_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
	}

	if (isFire_) {
		dxcommon_->GetDXCommand()->SetViewAndscissor();
		dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Fire);

		dxcommon_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexGrayBufferView_);
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(0, offTextureHandle_);
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(1, baseTex_->gpuHandle);
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, voronoTex_->gpuHandle);
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(3, noiseTex_->gpuHandle);
		dxcommon_->GetCommandList()->SetGraphicsRootConstantBufferView(4, fireResource_->GetGPUVirtualAddress());
		dxcommon_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
	}


	if (isThunder_) {
		dxcommon_->GetDXCommand()->SetViewAndscissor();
		dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Thunder);

		dxcommon_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexGrayBufferView_);
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(0, offTextureHandle_);
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(1, noiseDirTex_->gpuHandle);
		dxcommon_->GetCommandList()->SetGraphicsRootConstantBufferView(2, thunderResource_->GetGPUVirtualAddress());
		dxcommon_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
	}

	if (isCRT_) {
		dxcommon_->GetDXCommand()->SetViewAndscissor();
		dxcommon_->GetPipelineManager()->SetPipeline(Pipe::CRT);

		dxcommon_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexGrayBufferView_);
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(0, offTextureHandle_);
		dxcommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, cRTResource_->GetGPUVirtualAddress());
		dxcommon_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
	}

	if (isOutline_) {
		dxcommon_->PreOutline();
		dxcommon_->GetDXCommand()->SetViewAndscissor();
		dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Outline);

		dxcommon_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexGrayBufferView_);
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(0, offTextureHandle_);
		dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(1, dxcommon_->GetDepthTexGPUHandle());
		dxcommon_->GetCommandList()->SetGraphicsRootConstantBufferView(2, outlineResource_->GetGPUVirtualAddress());
		dxcommon_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
		dxcommon_->PostOutline();
	}
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
	device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&outputTexture_));


	// SRV/UAVの作成
	SRVManager* srvManager = SRVManager::GetInstance();
	outputSRVIndex_ = srvManager->Allocate();
	outputIndex_ = srvManager->Allocate();

	outputSRVHandleCPU_ = srvManager->GetCPUDescriptorHandle(outputSRVIndex_);
	outputSRVHandle_ = srvManager->GetGPUDescriptorHandle(outputSRVIndex_);
	outputUAVHandleCPU_ = srvManager->GetCPUDescriptorHandle(outputIndex_);
	outputUAVHandle_ = srvManager->GetGPUDescriptorHandle(outputIndex_);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	device->CreateShaderResourceView(outputTexture_.Get(), &srvDesc, outputSRVHandleCPU_);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	device->CreateUnorderedAccessView(outputTexture_.Get(), nullptr, &uavDesc, outputUAVHandleCPU_);
}
