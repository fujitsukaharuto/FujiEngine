#include "Model.h"




Model::Model() {
	data_.rootNode.local = MakeIdentity4x4();
	uvScale_ = { 1.0f,1.0f };
	uvTrans_ = { 0.0f,0.0f };
}

Model::~Model() {
	material_.clear();
	mesh_.clear();
}

void Model::Draw(ID3D12GraphicsCommandList* commandList, Material* mate = nullptr) {
	for (uint32_t index = 0; index < mesh_.size(); ++index) {
		if (mate) {
			commandList->SetGraphicsRootConstantBufferView(0, mate->GetMaterialResource()->GetGPUVirtualAddress());
			commandList->SetGraphicsRootDescriptorTable(2, mate->GetTexture()->gpuHandle);
		} else {
			commandList->SetGraphicsRootConstantBufferView(0, material_[index].GetMaterialResource()->GetGPUVirtualAddress());
			commandList->SetGraphicsRootDescriptorTable(2, material_[index].GetTexture()->gpuHandle);
		}
		mesh_[index].Draw(commandList);
	}
}

void Model::AnimationDraw(const SkinCluster& skinCluster, ID3D12GraphicsCommandList* commandList, Material* mate) {
	int vertexOffset = 0;
	for (uint32_t index = 0; index < mesh_.size(); ++index) {
		if (mate) {
			commandList->SetGraphicsRootConstantBufferView(0, mate->GetMaterialResource()->GetGPUVirtualAddress());
			commandList->SetGraphicsRootDescriptorTable(2, mate->GetTexture()->gpuHandle);
		} else {
			commandList->SetGraphicsRootConstantBufferView(0, material_[index].GetMaterialResource()->GetGPUVirtualAddress());
			commandList->SetGraphicsRootDescriptorTable(2, material_[index].GetTexture()->gpuHandle);
		}
		mesh_[index].AnimationDraw(skinCluster, commandList, vertexOffset);
		vertexOffset = 0;
		vertexOffset += int(mesh_[index].GetVertexDataSize());
	}
}

void Model::TransBarrier() {
	for (uint32_t index = 0; index < mesh_.size(); ++index) {
		mesh_[index].TransBarrier();
	}
}

void Model::AddMaterial(const Material& material) {
	material_.push_back(material);
}

void Model::AddMesh(const Mesh& mesh) {
	mesh_.push_back(mesh);
}

void Model::CreateEnvironment() {
	for (Material& material : material_) {
		material.CreateEnvironmentMaterial();
	}
	for (uint32_t index = 0; index < mesh_.size(); ++index) {
		mesh_[index].CreateUAV();
	}
}

void Model::CreateSkinningInformation(DXCom* pDxcom) {
	skinningInformation_ = pDxcom->CreateBufferResource(pDxcom->GetDevice(), sizeof(SkinningInformation));
	infoData_ = nullptr;
	skinningInformation_->Map(0, nullptr, reinterpret_cast<void**>(&infoData_));
	for (int i = 0; i < mesh_.size(); i++) {
		infoData_->numVertices += static_cast<int32_t>(mesh_[i].GetVertexDataSize());
	}
}

void Model::SetColor(const Vector4& color, int index) {
	material_[index].SetColor(color);
}

void Model::SetUVScale(const Vector2& scale, const Vector2& uvTrans) {
	uvScale_ = scale;
	uvTrans_ = uvTrans;
	for (Material& material : material_) {
		material.SetUVScale(scale, uvTrans);
	}
}

void Model::SetAlphaRef(float ref) {
	for (Material& material : material_) {
		material.SetAlphaRef(ref);
	}
}

void Model::SetEnvironment(float env) {
	for (Material& material : material_) {
		material.SetEnvironment(env);
	}
}

void Model::SetTexture(const std::string& name) {
	if (nowTextuer == name) {
		return;
	}
	for (uint32_t index = 0; index < mesh_.size(); ++index) {
		material_[index].SetTexture(name);
	}
	nowTextuer = name;
}

void Model::SetTextureName(const std::string& name) {
	nowTextuer = name;
}

void Model::SetLightEnable(LightMode mode) {
	for (uint32_t index = 0; index < mesh_.size(); ++index) {
		material_[index].SetLightEnable(mode);
	}
}

void Model::CSDispatch(const SkinCluster& skinCluster, ID3D12GraphicsCommandList* commandList) {
	PipelineManager::GetInstance()->SetCSPipeline(Pipe::SkinningCS);
	commandList->SetComputeRootDescriptorTable(0, skinCluster.paletteSrvHandle.second);        // t0
	commandList->SetComputeRootDescriptorTable(2, skinCluster.influenceSrvHandle.second);      // t1, t2
	commandList->SetComputeRootDescriptorTable(5, skinCluster.meshSectionSrvHandle.second);    // t3
	commandList->SetComputeRootConstantBufferView(4, skinningInformation_->GetGPUVirtualAddress()); // b0

	// 各メッシュセクションごとにDispatch
	for (uint32_t i = 0; i < static_cast<uint32_t>(skinCluster.meshSections.size()); ++i) {
		// メッシュ側でSRVなどセット（頂点バッファやスキン出力先）
		mesh_[i].CSDispatch(commandList);

		// RootConstantで meshIndex を送信（b1）
		commandList->SetComputeRoot32BitConstants(6, 1, &i, 0);

		uint32_t vertexCount = skinCluster.meshSections[i].vertexCount;
		uint32_t dispatchCount = (vertexCount + 1023) / 1024;

		commandList->Dispatch(dispatchCount, 1, 1);
	}
}

void Model::MeshDraw(ID3D12GraphicsCommandList* commandList, Material* mate, int drawCount) {
	for (uint32_t index = 0; index < mesh_.size(); ++index) {
		if (mate) {
			commandList->SetGraphicsRootConstantBufferView(0, mate->GetMaterialResource()->GetGPUVirtualAddress());
			commandList->SetGraphicsRootDescriptorTable(2, mate->GetTexture()->gpuHandle);
		} else {
			commandList->SetGraphicsRootConstantBufferView(0, material_[index].GetMaterialResource()->GetGPUVirtualAddress());
			commandList->SetGraphicsRootDescriptorTable(2, material_[index].GetTexture()->gpuHandle);
		}
		mesh_[index].MeshDraw(commandList, drawCount);
	}
}
