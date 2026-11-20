#include "SkyBox.h"
#include "Engine/Graphics/Pipeline/RootNames.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/Graphics/Object/ObjectRenderer.h"
#include "Engine/Graphics/Model/ModelManager.h"
#include "Engine/Graphics/Pipeline/PipelineManager.h"
#include "Engine/Core/App/MyWindow.h"
#include "Engine/Graphics/Texture/TextureManager.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Light/LightManager.h"
#include "Engine/Graphics/SkyBox/SkyAtmosphere.h"
#include "Engine/Graphics/Raytracing/VolumetricFogPass.h"
#include <algorithm>
#include <cmath>
#include <string>
#include "externals/DirectXTex/DirectXTex.h"

using namespace Core;
using namespace Graphics;
using namespace Math;
using namespace DXC;


namespace {
	const std::string kTextureName = "skyboxTexture.dds";
}

SkyBox::SkyBox() {
	// Object3d(ModelManager) / Sprite(TextureManager) と同じく、共有物はシングルトンから取る。
	// 呼び出し側に配線させない
	dxcommon_ = ModelManager::GetInstance()->ShareDXCom();
	srvManager_ = SRVManager::GetInstance();
	camera_ = CameraManager::GetInstance()->GetCamera();
}

SkyBox::~SkyBox() {
}

void SkyBox::Initialize() {
	ResourceCreate();

	transform_.scale = { 300.0f,300.0f,300.0f };
}

void SkyBox::Draw() {
	UpdateWVP();
	ObjectRenderer::GetInstance()->SetSkyBox(this);
}

void SkyBox::Render() {
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();

	dxcommon_->GetDXCommand()->SetViewAndScissor(MyWin::kWindowWidth, MyWin::kWindowHeight);
	pPipeManager->SetPipeline(Pipe::Skybox);
	cList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cList->IASetVertexBuffers(0, 1, &vbView);
	cList->IASetIndexBuffer(&ibView);

	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	pPipeManager->SetGraphicsRootCBV(cList, RootName::kMaterial, material_.UploadAndGetResource()->GetGPUVirtualAddress());
	pPipeManager->SetGraphicsRootCBV(cList, RootName::kTransformationMatrix, wvpResource_[frameIndex]->GetGPUVirtualAddress());
	pPipeManager->SetGraphicsRootDescriptorTable(cList, RootName::kTexture, material_.GetTexture()->gpuHandle);
	// PS が gFog を無条件に宣言しているので、フォグを切っていてもバインドが要る
	pPipeManager->SetGraphicsRootCBV(cList, RootName::kFog,
		ObjectRenderer::GetInstance()->GetLightManager()->GetFogGPUVirtualAddress());
	// PS が宣言している。テクスチャの空のときは引かれない
	pPipeManager->SetGraphicsRootDescriptorTable(cList, RootName::kSkyViewLut,
		ObjectRenderer::GetInstance()->GetSkyAtmosphere()->GetSkyViewSrvHandle());

	// ボリュメトリックフォグの表。作っていないフレームもバインドが要る
	if (const VolumetricFogPass* fog = ObjectRenderer::GetInstance()->GetVolumetricFogPass()) {
		pPipeManager->SetGraphicsRootCBV(cList, RootName::kVolumetricFog, fog->GetParamsGPUVirtualAddress());
		pPipeManager->SetGraphicsRootDescriptorTable(cList, RootName::kVolumetricFogVolume, fog->GetVolumeSrvHandle());
		pPipeManager->SetGraphicsRootDescriptorTable(cList, RootName::kLocalFogVolumes, fog->GetLocalFogVolumesSrvHandle());
	}

	cList->DrawIndexedInstanced(static_cast<UINT>((index_.size())), 1, 0, 0, 0);
}

void SkyBox::DebugGUI() {
#ifdef _DEBUGMODE
	if (ImGui::CollapsingHeader("SkyBox")) {
		ImGui::Indent();
		Vector4 color = material_.GetColor();
		ImGui::ColorEdit3("color##SkyBoxColor", &color.x);
		material_.SetColor(color);
		ImGui::Unindent();
	}
#endif // _DEBUG
}

void SkyBox::UpdateWVP() {
	Matrix4x4 worldViewProjectionMatrix;
	// 箱はカメラを中心に置く。原点に置いたままだと、カメラが箱 (300m) の外へ出たとき空が描かれない。
	// 空の向きは頂点の向きから決めているので、動かしても見え方は変わらない
	const Vector3 center = camera_ ? camera_->GetTranslate() : transform_.translate;
	const Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale, transform_.rotate, center);

	if (camera_) {
		const Matrix4x4& viewProjectionMatrix = camera_->GetViewProjectionMatrix();
		worldViewProjectionMatrix = Multiply(worldMatrix, viewProjectionMatrix);
	} else {
		worldViewProjectionMatrix = worldMatrix;
	}

	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	wvpDataGPU_[frameIndex]->World = worldMatrix;
	wvpDataGPU_[frameIndex]->WVP = worldViewProjectionMatrix;
	wvpDataGPU_[frameIndex]->WorldInverseTransPose = Transpose(Inverse(worldMatrix));

}

void SkyBox::SetColor(const Math::Vector4& color) {
	material_.SetColor(color);
}

Vector3 SkyBox::GetHorizonColor(int side) {
	const Vector4 tint = material_.GetColor();
	// Skybox.PS と同じ露光とトーンマップ。向こうを変えたらここも合わせること
	auto shade = [](float x) {
		x *= 0.25f;
		const float y = (x * (2.51f * x + 0.03f)) / (x * (2.43f * x + 0.59f) + 0.14f);
		return std::clamp(y, 0.0f, 1.0f);
	};
	const Vector3& raw = horizonColor_[side];
	return { shade(raw.x * tint.x), shade(raw.y * tint.y), shade(raw.z * tint.z) };
}

void SkyBox::MeasureHorizonColor() {
	// シーンに入るたびに SkyBox が作られるので、測った値は使い回す
	static bool isMeasured = false;
	static Vector3 measured[4]{};
	if (!isMeasured) {
		isMeasured = true;
		// 読めなかったときは灰色。真っ黒のフォグになるよりは気付きやすい
		for (Vector3& color : measured) {
			color = { 0.5f,0.5f,0.5f };
		}

		const std::string path = "resource/Textures/" + kTextureName;
		const std::wstring pathW(path.begin(), path.end());
		DirectX::ScratchImage image;
		DirectX::TexMetadata meta{};
		if (SUCCEEDED(DirectX::LoadFromDDSFile(pathW.c_str(), DirectX::DDS_FLAGS_NONE, &meta, image)) && meta.IsCubemap()) {
			// 平均を取るだけなので 32x32 あたりのミップで足りる
			size_t mip = 0;
			while (mip + 1 < meta.mipLevels && (meta.width >> mip) > 32) {
				++mip;
			}
			// キューブマップの面は +X, -X, +Y, -Y, +Z, -Z の順
			const size_t kSideFaces[4] = { 0,1,4,5 };
			for (int side = 0; side < 4; ++side) {
				const DirectX::Image* source = image.GetImage(mip, kSideFaces[side], 0);
				if (!source) {
					continue;
				}
				DirectX::ScratchImage converted;
				HRESULT hr = S_OK;
				if (DirectX::IsCompressed(source->format)) {
					hr = DirectX::Decompress(*source, DXGI_FORMAT_R32G32B32A32_FLOAT, converted);
				} else if (source->format != DXGI_FORMAT_R32G32B32A32_FLOAT) {
					hr = DirectX::Convert(*source, DXGI_FORMAT_R32G32B32A32_FLOAT, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, converted);
				}
				if (FAILED(hr)) {
					continue;
				}
				const DirectX::Image* pixels = converted.GetImageCount() > 0 ? converted.GetImage(0, 0, 0) : source;

				// 横の面は真ん中の行が地平線。その上下 1/16 ずつの帯を平均する
				const size_t rowBegin = pixels->height * 7 / 16;
				const size_t rowEnd = (std::max)(pixels->height * 9 / 16, rowBegin + 1);
				double sum[3]{};
				for (size_t y = rowBegin; y < rowEnd; ++y) {
					const float* row = reinterpret_cast<const float*>(pixels->pixels + y * pixels->rowPitch);
					for (size_t x = 0; x < pixels->width; ++x) {
						sum[0] += row[x * 4 + 0];
						sum[1] += row[x * 4 + 1];
						sum[2] += row[x * 4 + 2];
					}
				}
				const double count = static_cast<double>((rowEnd - rowBegin) * pixels->width);
				measured[side] = { static_cast<float>(sum[0] / count), static_cast<float>(sum[1] / count), static_cast<float>(sum[2] / count) };
			}
		}
	}
	for (int i = 0; i < 4; ++i) {
		horizonColor_[i] = measured[i];
	}
}

void SkyBox::ResourceCreate() {

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		wvpResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(TransformationMatrix));
		wvpDataGPU_[i] = nullptr;
		wvpResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&wvpDataGPU_[i]));
	
		wvpDataGPU_[i]->WVP = MakeIdentity4x4();
		wvpDataGPU_[i]->World = MakeIdentity4x4();
		wvpDataGPU_[i]->WorldInverseTransPose = MakeIdentity4x4();
	}

	material_.SetTextureNamePath(kTextureName);
	material_.CreateMaterial();
	MeasureHorizonColor();

	CreateVertex();
}

void SkyBox::CreateVertex() {
	// 右
	vertex_.push_back({ {1.0f,1.0f,1.0f,1.0f},{0.0f,0.0f},{-1.0f,0.0f,0.0f} });
	vertex_.push_back({ {1.0f,1.0f,-1.0f,1.0f},{0.0f,1.0f},{-1.0f,0.0f,0.0f} });
	vertex_.push_back({ {1.0f,-1.0f,1.0f,1.0f},{1.0f,1.0f},{-1.0f,0.0f,0.0f} });
	vertex_.push_back({ {1.0f,-1.0f,-1.0f,1.0f},{1.0f,0.0f},{-1.0f,0.0f,0.0f} });

	index_.push_back(0);
	index_.push_back(1);
	index_.push_back(2);

	index_.push_back(2);
	index_.push_back(1);
	index_.push_back(3);

	// 左
	vertex_.push_back({ {-1.0f,1.0f,-1.0f,1.0f},{0.0f,0.0f},{1.0f,0.0f,0.0f} });
	vertex_.push_back({ {-1.0f,1.0f,1.0f,1.0f},{0.0f,1.0f},{1.0f,0.0f,0.0f} });
	vertex_.push_back({ {-1.0f,-1.0f,-1.0f,1.0f},{1.0f,1.0f},{1.0f,0.0f,0.0f} });
	vertex_.push_back({ {-1.0f,-1.0f,1.0f,1.0f},{1.0f,0.0f},{1.0f,0.0f,0.0f} });

	index_.push_back(4);
	index_.push_back(5);
	index_.push_back(6);

	index_.push_back(6);
	index_.push_back(5);
	index_.push_back(7);

	// 前
	vertex_.push_back({ {-1.0f,1.0f,1.0f,1.0f},{0.0f,0.0f},{0.0f,0.0f,-1.0f} });
	vertex_.push_back({ {1.0f,1.0f,1.0f,1.0f},{0.0f,1.0f},{0.0f,0.0f,-1.0f} });
	vertex_.push_back({ {-1.0f,-1.0f,1.0f,1.0f},{1.0f,1.0f},{0.0f,0.0f,-1.0f} });
	vertex_.push_back({ {1.0f,-1.0f,1.0f,1.0f},{1.0f,0.0f},{0.0f,0.0f,-1.0f} });

	index_.push_back(8);
	index_.push_back(9);
	index_.push_back(10);

	index_.push_back(10);
	index_.push_back(9);
	index_.push_back(11);

	// 後
	vertex_.push_back({ {1.0f,1.0f,-1.0f,1.0f},{0.0f,0.0f},{0.0f,0.0f,1.0f} });
	vertex_.push_back({ {-1.0f,1.0f,-1.0f,1.0f},{0.0f,1.0f},{0.0f,0.0f,1.0f} });
	vertex_.push_back({ {1.0f,-1.0f,-1.0f,1.0f},{1.0f,1.0f},{0.0f,0.0f,1.0f} });
	vertex_.push_back({ {-1.0f,-1.0f,-1.0f,1.0f},{1.0f,0.0f},{0.0f,0.0f,1.0f} });

	index_.push_back(12);
	index_.push_back(13);
	index_.push_back(14);

	index_.push_back(14);
	index_.push_back(13);
	index_.push_back(15);

	// 上
	vertex_.push_back({ {-1.0f,1.0f,-1.0f,1.0f},{0.0f,0.0f},{0.0f,-1.0f,0.0f} });
	vertex_.push_back({ {1.0f,1.0f,-1.0f,1.0f},{0.0f,1.0f},{0.0f,-1.0f,0.0f} });
	vertex_.push_back({ {-1.0f,1.0f,1.0f,1.0f},{1.0f,1.0f},{0.0f,-1.0f,0.0f} });
	vertex_.push_back({ {1.0f,1.0f,1.0f,1.0f},{1.0f,0.0f},{0.0f,-1.0f,0.0f} });

	index_.push_back(16);
	index_.push_back(17);
	index_.push_back(18);

	index_.push_back(18);
	index_.push_back(17);
	index_.push_back(19);

	// 下
	vertex_.push_back({ {-1.0f,-1.0f,1.0f,1.0f},{0.0f,0.0f},{0.0f,1.0f,0.0f} });
	vertex_.push_back({ {1.0f,-1.0f,1.0f,1.0f},{0.0f,1.0f},{0.0f,1.0f,0.0f} });
	vertex_.push_back({ {-1.0f,-1.0f,-1.0f,1.0f},{1.0f,1.0f},{0.0f,1.0f,0.0f} });
	vertex_.push_back({ {1.0f,-1.0f,-1.0f,1.0f},{1.0f,0.0f},{0.0f,1.0f,0.0f} });

	index_.push_back(20);
	index_.push_back(21);
	index_.push_back(22);

	index_.push_back(22);
	index_.push_back(21);
	index_.push_back(23);


	vBuffer_ = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(VertexData) * vertex_.size());
	iBuffer_ = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(uint32_t) * index_.size());

	VertexData* vData = nullptr;
	vBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&vData));
	std::memcpy(vData, vertex_.data(), sizeof(VertexData) * vertex_.size());

	vbView.BufferLocation = vBuffer_->GetGPUVirtualAddress();
	vbView.SizeInBytes = static_cast<UINT>(sizeof(VertexData) * vertex_.size());
	vbView.StrideInBytes = static_cast<UINT>(sizeof(VertexData));

	uint32_t* iData = nullptr;
	iBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&iData));
	std::memcpy(iData, index_.data(), sizeof(uint32_t) * index_.size());

	ibView.BufferLocation = iBuffer_->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R32_UINT;
	ibView.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * index_.size());


}
