#include "SkyBox.h"

SkyBox::SkyBox() {
}

SkyBox::~SkyBox() {
}

void SkyBox::Initialize() {
	ResourceCreate();

	transform_.scale = { 300.0f,300.0f,300.0f };
}

void SkyBox::Update() {
	UpdateWVP();
}

void SkyBox::Draw() {
	dxcommon_->GetDXCommand()->SetViewAndscissor();
	dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Skybox);
	dxcommon_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vbView);
	dxcommon_->GetCommandList()->IASetIndexBuffer(&ibView);

	dxcommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, material_.GetMaterialResource()->GetGPUVirtualAddress());
	dxcommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
	dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, material_.GetTexture()->gpuHandle);
	
	dxcommon_->GetCommandList()->DrawIndexedInstanced(static_cast<UINT>((index_.size())), 1, 0, 0, 0);
}

void SkyBox::DebugGUI() {
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

	wvpDate_->World = worldMatrix;
	wvpDate_->WVP = worldViewProjectionMatrix;
	wvpDate_->WorldInverseTransPose = Transpose(Inverse(wvpDate_->World));

}

void SkyBox::SetCommonResources(DXCom* dxcommon, SRVManager* srvManager, Camera* camera) {
	dxcommon_ = dxcommon;
	srvManager_ = srvManager;
	camera_ = camera;
}

void SkyBox::ResourceCreate() {

	wvpResource_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(TransformationMatrix));
	wvpDate_ = nullptr;
	wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpDate_));
	wvpDate_->WVP = MakeIdentity4x4();
	wvpDate_->World = MakeIdentity4x4();
	wvpDate_->WorldInverseTransPose = Transpose(Inverse(wvpDate_->World));

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


	vBuffer_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(VertexDate) * vertex_.size());
	iBuffer_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(uint32_t) * index_.size());

	VertexDate* vData = nullptr;
	vBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&vData));
	std::memcpy(vData, vertex_.data(), sizeof(VertexDate) * vertex_.size());

	vbView.BufferLocation = vBuffer_->GetGPUVirtualAddress();
	vbView.SizeInBytes = static_cast<UINT>(sizeof(VertexDate) * vertex_.size());
	vbView.StrideInBytes = static_cast<UINT>(sizeof(VertexDate));

	uint32_t* iData = nullptr;
	iBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&iData));
	std::memcpy(iData, index_.data(), sizeof(uint32_t) * index_.size());

	ibView.BufferLocation = iBuffer_->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R32_UINT;
	ibView.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * index_.size());


}
