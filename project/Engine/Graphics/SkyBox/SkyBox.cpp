#include "SkyBox.h"
#include "Engine/Graphics/Pipeline/RootNames.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/Graphics/Object/ObjectRenderer.h"
#include "Engine/Graphics/Model/ModelManager.h"
#include "Engine/Graphics/Pipeline/PipelineManager.h"
#include "Engine/Core/App/MyWindow.h"

using namespace Core;
using namespace Graphics;
using namespace Math;
using namespace DXC;


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
	pPipeManager->SetGraphicsRootCBV(cList, RootName::kMaterial, material_.GetMaterialResource()->GetGPUVirtualAddress());
	pPipeManager->SetGraphicsRootCBV(cList, RootName::kTransformationMatrix, wvpResource_[frameIndex]->GetGPUVirtualAddress());
	pPipeManager->SetGraphicsRootDescriptorTable(cList, RootName::kTexture, material_.GetTexture()->gpuHandle);

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
	Matrix4x4 worldMatrix = MakeIdentity4x4();
	worldMatrix = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);

	if (camera_) {
		const Matrix4x4& viewProjectionMatrix = camera_->GetViewProjectionMatrix();
		worldViewProjectionMatrix = Multiply(worldMatrix, viewProjectionMatrix);
	} else {
		worldViewProjectionMatrix = worldMatrix;
	}

	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	wvpDataGPU_[frameIndex]->World = worldMatrix;
	wvpDataGPU_[frameIndex]->WVP = worldViewProjectionMatrix;
	wvpDataGPU_[frameIndex]->WorldInverseTransPose = Transpose(Inverse(wvpDataGPU_[frameIndex]->World));

}

void SkyBox::SetColor(const Math::Vector4& color) {
	material_.SetColor(color);
}

void SkyBox::ResourceCreate() {

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		wvpResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(TransformationMatrix));
		wvpDataGPU_[i] = nullptr;
		wvpResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&wvpDataGPU_[i]));
	
		wvpDataGPU_[i]->WVP = MakeIdentity4x4();
		wvpDataGPU_[i]->World = MakeIdentity4x4();
		wvpDataGPU_[i]->WorldInverseTransPose = Transpose(Inverse(wvpDataGPU_[i]->World));
	}

	material_.SetTextureNamePath("skyboxTexture.dds");
	material_.CreateMaterial();

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
