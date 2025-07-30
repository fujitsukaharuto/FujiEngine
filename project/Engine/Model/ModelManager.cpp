#include "ModelManager.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Engine/DX/SRVManager.h"
#include "Engine/WinApp/MyWindow.h"
#include "Engine/ImGuiManager/ImGuiManager.h"
#include "Engine/Input/Input.h"
#include "Engine/Light/LightManager.h"
#ifdef _DEBUG
#include "ImGuizmo.h"
#endif // _DEBUG



ModelManager::~ModelManager() {
}


ModelManager* ModelManager::GetInstance() {
	static ModelManager instance;
	return &instance;
}


void ModelManager::Initialize(DXCom* pDxcom, LightManager* pLight) {
	dxcommon_ = pDxcom;
	lightManager_ = pLight;
	LoadModelFile();

	pickingBufferResource_ = dxcommon_->CreateUAVResource(dxcommon_->GetDevice(), sizeof(PickingBuffer));

	uint32_t srvIndex = SRVManager::GetInstance()->Allocate();
	pickBufferHandle_.first = SRVManager::GetInstance()->GetCPUDescriptorHandle(srvIndex);
	pickBufferHandle_.second = SRVManager::GetInstance()->GetGPUDescriptorHandle(srvIndex);
	SRVManager::GetInstance()->CreateStructuredUAV(srvIndex, pickingBufferResource_.Get(),
		static_cast<UINT>(1), static_cast<UINT>(sizeof(PickingBuffer)));

	pickingBufferReadBack_ = dxcommon_->CreateReadbackResource(dxcommon_->GetDevice(), sizeof(PickingBuffer));


	pickingDataResource_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(PickingData));
	pickingData_ = nullptr;
	pickingDataResource_->Map(0, nullptr, reinterpret_cast<void**>(&pickingData_));
	pickingData_->pickingEnable = 0;
	pickingData_->pickingPixelCoord[0] = -1;
	pickingData_->pickingPixelCoord[1] = -1;


	lastPicked_.objID = -1;
	lastPicked_.depth = 1.0f;
	PickingBuffer init{};
	init.objID = -1;
	init.depth = 1.0f;
	initUploadBuffer_ = dxcommon_->CreateUploadBuffer(sizeof(PickingBuffer), &init);

	dxcommon_->GetCommandList()->CopyResource(pickingBufferResource_.Get(), initUploadBuffer_.Get());
	dxcommon_->CommandExecution();
}


void ModelManager::Finalize() {
	dxcommon_ = nullptr;

	pickingBufferResource_.Reset();
	pickingBufferReadBack_.Reset();
	pickingDataResource_.Reset();
	initUploadBuffer_.Reset();

	modelFileList.clear();
	models_.clear();
}


void ModelManager::LoadModelByExtension(const std::string& filename, bool overWrite) {
	std::filesystem::path path(filename);  // 拡張子の解析に便利
	std::string ext = path.extension().string();

	if (ext == ".obj") {
		LoadOBJ(filename, overWrite);
	} else if (ext == ".gltf") {
		LoadGLTF(filename, overWrite);
	}
}

void ModelManager::LoadOBJ(const std::string& filename, bool overWrite) {
	ModelManager* instance = GetInstance();
	if (!overWrite) {
		auto iterator = instance->models_.find(filename);
		if (iterator != instance->models_.end()) {
			return;
		}
	}

	std::unique_ptr<Model> model;
	model.reset(new Model());

	Assimp::Importer importer;
	std::string path = instance->kDirectoryPath_ + filename;
	const aiScene* scene = importer.ReadFile(path.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs);
	assert(scene->HasMeshes());

	// Mesh解析
	uint32_t meshVertexCount = 0;
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++) {
		aiMesh* mesh = scene->mMeshes[meshIndex];
		assert(mesh->HasNormals());
		bool hasTexcoord = mesh->HasTextureCoords(0);

		ModelMesh newModelMesh{};
		// Vertex解析
		for (uint32_t element = 0; element < mesh->mNumVertices; element++) {

			aiVector3D& position = mesh->mVertices[element];
			aiVector3D& normal = mesh->mNormals[element];

			VertexDate vertex;
			vertex.position = { position.x,position.y,position.z,1.0f };
			vertex.normal = { normal.x,normal.y,normal.z };

			if (hasTexcoord) {
				aiVector3D& texcoord = mesh->mTextureCoords[0][element];
				vertex.texcoord = { texcoord.x,texcoord.y };
			} else {
				vertex.texcoord = { 0.0f,0.0f };
			}

			vertex.position.x *= -1.0f;
			vertex.normal.x *= -1.0f;

			//newMesh.AddVertex({ {vertex.position},{vertex.texcoord},{vertex.normal} });
			model->data_.vertices.push_back({ {vertex.position},{vertex.texcoord},{vertex.normal} });
			newModelMesh.vertices.push_back({ {vertex.position},{vertex.texcoord},{vertex.normal} });
		}

		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++) {
			aiFace& face = mesh->mFaces[faceIndex];
			assert(face.mNumIndices == 3);

			for (uint32_t element = 0; element < face.mNumIndices; element++) {
				uint32_t vertexIndex = face.mIndices[element];
				//newMesh.AddIndex(vertexIndex + meshVertexCount);
				model->data_.indicies.push_back(vertexIndex);
				newModelMesh.indicies.push_back(vertexIndex);
			}
		}
		meshVertexCount += mesh->mNumVertices;

		// === メッシュに対応するマテリアルを取得 ===
		uint32_t materialIndex = mesh->mMaterialIndex;
		aiMaterial* material = scene->mMaterials[materialIndex];

		aiString textureFileName;
		std::string texturePath;

		aiColor3D diffuseColor(1.0f, 1.0f, 1.0f);
		if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0 &&
			material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFileName) == AI_SUCCESS) {
			texturePath = textureFileName.C_Str();
			newModelMesh.baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		} else {
			if (material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) == AI_SUCCESS) {
				newModelMesh.baseColor = { diffuseColor.r, diffuseColor.g, diffuseColor.b, 1.0f };
			} else {
				newModelMesh.baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
			}
			texturePath = "white2x2.png";  // デフォルト
		}
		newModelMesh.material.textureFilePath = texturePath;

		model->data_.meshes.push_back(newModelMesh);
	}

	if (overWrite) {
		instance->models_[filename] = std::move(model);
	} else {
		instance->models_.insert(std::make_pair(filename, std::move(model)));
	}
}

void ModelManager::LoadGLTF(const std::string& filename, bool overWrite) {
	ModelManager* instance = GetInstance();
	if (!overWrite) {
		auto iterator = instance->models_.find(filename);
		if (iterator != instance->models_.end()) {
			return;
		}
	}

	std::unique_ptr<Model> model;
	model.reset(new Model());

	Assimp::Importer importer;
	std::string path = instance->kDirectoryPath_ + "/" + filename;
	const aiScene* scene = importer.ReadFile(path.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs);
	assert(scene->HasMeshes());

	// Mesh解析
	uint32_t meshVertexCount = 0;
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++) {
		ModelMesh newModelMesh{};
		aiMesh* mesh = scene->mMeshes[meshIndex];
		assert(mesh->HasNormals());
		bool hasTexcoord = mesh->HasTextureCoords(0);

		// Vertex解析
		for (uint32_t element = 0; element < mesh->mNumVertices; element++) {
			aiVector3D& position = mesh->mVertices[element];
			aiVector3D& normal = mesh->mNormals[element];

			VertexDate vertex;
			vertex.position = { position.x,position.y,position.z,1.0f };
			vertex.normal = { normal.x,normal.y,normal.z };

			if (hasTexcoord) {
				aiVector3D& texcoord = mesh->mTextureCoords[0][element];
				vertex.texcoord = { texcoord.x,texcoord.y };
			} else {
				vertex.texcoord = { 0.0f,0.0f };
			}

			vertex.position.x *= -1.0f;
			vertex.normal.x *= -1.0f;

			model->data_.vertices.push_back({ {vertex.position},{vertex.texcoord},{vertex.normal} });
			newModelMesh.vertices.push_back({ {vertex.position},{vertex.texcoord},{vertex.normal} });
		}

		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++) {
			aiFace& face = mesh->mFaces[faceIndex];
			assert(face.mNumIndices == 3);

			for (uint32_t element = 0; element < face.mNumIndices; element++) {
				uint32_t vertexIndex = face.mIndices[element];
				model->data_.indicies.push_back(vertexIndex);
				newModelMesh.indicies.push_back(vertexIndex);
			}
		}

		// === メッシュに対応するマテリアルを取得 ===
		uint32_t materialIndex = mesh->mMaterialIndex;
		aiMaterial* material = scene->mMaterials[materialIndex];

		aiString textureFileName;
		std::string texturePath;

		aiColor3D diffuseColor(1.0f, 1.0f, 1.0f);
		if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0 &&
			material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFileName) == AI_SUCCESS) {
			texturePath = textureFileName.C_Str();
			newModelMesh.baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		} else {
			if (material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) == AI_SUCCESS) {
				newModelMesh.baseColor = { diffuseColor.r, diffuseColor.g, diffuseColor.b, 1.0f };
			} else {
				newModelMesh.baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
			}
			texturePath = "white2x2.png";  // デフォルト
		}
		newModelMesh.material.textureFilePath = texturePath;
		model->data_.meshes.push_back(newModelMesh);

		// SkinCluster構築用のデータ取得
		for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++) {
			aiBone* bone = mesh->mBones[boneIndex];
			std::string jointName = bone->mName.C_Str();
			JointWeightData& jointWeightData = model->data_.skinClusterData[jointName];

			aiMatrix4x4 bindPosMatrixAssimp = bone->mOffsetMatrix.Inverse();
			aiVector3D scale, translate;
			aiQuaternion rotate;
			bindPosMatrixAssimp.Decompose(scale, rotate, translate);
			Matrix4x4 bindPoseMatrix = MakeAffineMatrix(
				{ scale.x,scale.y,scale.z }, { rotate.x,-rotate.y,-rotate.z,rotate.w }, { -translate.x,translate.y,translate.z }
			);
			jointWeightData.inverseBindPoseMatrix = Inverse(bindPoseMatrix);

			for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; weightIndex++) {
				uint32_t localIndex = bone->mWeights[weightIndex].mVertexId;
				uint32_t globalIndex = meshVertexCount + localIndex;

				jointWeightData.vertexWeights.push_back({
					bone->mWeights[weightIndex].mWeight,
					globalIndex
					});
			}
		}
		meshVertexCount += mesh->mNumVertices;
	}
	model->data_.rootNode = ReadNode(scene->mRootNode);

	if (overWrite) {
		instance->models_[filename] = std::move(model);
	} else {
		instance->models_.insert(std::make_pair(filename, std::move(model)));
	}
}


ModelData ModelManager::FindModel(const std::string& filename, bool overWrite) {
	ModelManager* instance = GetInstance();
	instance->LoadModelByExtension(filename, overWrite);
	auto iterator = instance->models_.find(filename);
	if (iterator != instance->models_.end()) {
		return iterator->second->data_;
	}
	return ModelData{};
}


void ModelManager::CreateSphere() {
	ModelManager* instance = GetInstance();

	auto iterator = instance->models_.find("Sphere");
	if (iterator != instance->models_.end()) {
		return;
	}

	std::unique_ptr<Model> model;
	model.reset(new Model());
	ModelMesh newModelMesh{};

	const float pi = 3.1415926535f;
	const uint32_t kSubdivision = 16;

	const float kLonEvery = (2.0f * pi) / static_cast<float>(kSubdivision);
	const float kLatEvery = pi / static_cast<float>(kSubdivision);

	// 頂点生成
	for (uint32_t latIndex = 0; latIndex <= kSubdivision; ++latIndex) {
		float lat = -pi / 2.0f + kLatEvery * latIndex;
		float v = 1.0f - float(latIndex) / float(kSubdivision); // 上がv=0, 下がv=1になるように

		for (uint32_t lonIndex = 0; lonIndex <= kSubdivision; ++lonIndex) {
			// 経度ループ用に +1 まで回す
			float lon = lonIndex * kLonEvery;
			float u = float(lonIndex) / float(kSubdivision); // 経度でuを算出（0〜1）

			float x = cosf(lat) * cosf(lon);
			float y = sinf(lat);
			float z = cosf(lat) * sinf(lon);

			model->data_.vertices.push_back({ {x, y, z, 1.0f},{u, v},{x, y, z} });
			newModelMesh.vertices.push_back({ {x, y, z, 1.0f},{u, v},{x, y, z} });
		}
	}

	// インデックス生成
	for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
			uint32_t row1 = latIndex * (kSubdivision + 1);
			uint32_t row2 = (latIndex + 1) * (kSubdivision + 1);

			uint32_t v0 = row1 + lonIndex;
			uint32_t v1 = row1 + lonIndex + 1;
			uint32_t v2 = row2 + lonIndex;
			uint32_t v3 = row2 + lonIndex + 1;

			model->data_.indicies.push_back(v0);
			newModelMesh.indicies.push_back(v0);
			model->data_.indicies.push_back(v2);
			newModelMesh.indicies.push_back(v2);
			model->data_.indicies.push_back(v1);
			newModelMesh.indicies.push_back(v1);

			model->data_.indicies.push_back(v1);
			newModelMesh.indicies.push_back(v1);
			model->data_.indicies.push_back(v2);
			newModelMesh.indicies.push_back(v2);
			model->data_.indicies.push_back(v3);
			newModelMesh.indicies.push_back(v3);
		}
	}
	model->data_.meshes.push_back(newModelMesh);

	instance->models_.insert(std::make_pair("Sphere", std::move(model)));
}

ModelData ModelManager::CreateRing(float out, float in, float radius, bool horizon) {
	std::unique_ptr<Model> model;
	model.reset(new Model());
	ModelMesh newModelMesh{};

	const uint32_t kRingDivide = 32;
	const float kOuterRadius = out;
	const float kInnerRadius = in;
	const float radianPerDivide = radius * std::numbers::pi_v<float> / float(kRingDivide);

	for (uint32_t i = 0; i <= kRingDivide; i++) {
		float angle = i * radianPerDivide;
		float sinA = std::sin(angle);
		float cosA = std::cos(angle);
		float u = float(i) / float(kRingDivide);

		if (horizon) {
			// 外周
			model->data_.vertices.push_back({ {-sinA * kOuterRadius, 0.0f, cosA * kOuterRadius, 1.0f}, {u, 0.0f}, {0,0,1} });
			newModelMesh.vertices.push_back({ {-sinA * kOuterRadius, 0.0f, cosA * kOuterRadius, 1.0f}, {u, 0.0f}, {0,0,1} });
			// 内周
			model->data_.vertices.push_back({ {-sinA * kInnerRadius, 0.0f, cosA * kInnerRadius, 1.0f}, {u, 1.0f}, {0,0,1} });
			newModelMesh.vertices.push_back({ {-sinA * kInnerRadius, 0.0f, cosA * kInnerRadius, 1.0f}, {u, 1.0f}, {0,0,1} });
		} else {
			// 外周
			model->data_.vertices.push_back({ {-sinA * kOuterRadius, cosA * kOuterRadius, 0.0f, 1.0f}, {u, 0.0f}, {0,0,1} });
			newModelMesh.vertices.push_back({ {-sinA * kOuterRadius, cosA * kOuterRadius, 0.0f, 1.0f}, {u, 0.0f}, {0,0,1} });
			// 内周
			model->data_.vertices.push_back({ {-sinA * kInnerRadius, cosA * kInnerRadius, 0.0f, 1.0f}, {u, 1.0f}, {0,0,1} });
			newModelMesh.vertices.push_back({ {-sinA * kInnerRadius, cosA * kInnerRadius, 0.0f, 1.0f}, {u, 1.0f}, {0,0,1} });
		}
	}

	// インデックス生成
	for (uint32_t i = 0; i < kRingDivide; i++) {
		uint32_t outer0 = i * 2;
		uint32_t inner0 = outer0 + 1;
		uint32_t outer1 = outer0 + 2;
		uint32_t inner1 = outer0 + 3;

		// 三角形1
		model->data_.indicies.push_back(outer0);
		newModelMesh.indicies.push_back(outer0);
		model->data_.indicies.push_back(inner0);
		newModelMesh.indicies.push_back(inner0);
		model->data_.indicies.push_back(outer1);
		newModelMesh.indicies.push_back(outer1);

		// 三角形2
		model->data_.indicies.push_back(outer1);
		newModelMesh.indicies.push_back(outer1);
		model->data_.indicies.push_back(inner0);
		newModelMesh.indicies.push_back(inner0);
		model->data_.indicies.push_back(inner1);
		newModelMesh.indicies.push_back(inner1);
	}
	model->data_.meshes.push_back(newModelMesh);

	return model->data_;
}

ModelData ModelManager::CreateCylinder(float topRadius, float bottomRadius, float height) {
	std::unique_ptr<Model> model;
	model.reset(new Model());
	ModelMesh newModelMesh{};

	const uint32_t kCylinderDivide = 32;
	const float kTopRadius = topRadius;
	const float kBottomRadius = bottomRadius;
	const float kHeight = height;
	const float radianPerDivide = 2.0f * std::numbers::pi_v<float> / float(kCylinderDivide);

	for (uint32_t i = 0; i <= kCylinderDivide; i++) {
		float angle = i * radianPerDivide;
		float sinA = std::sin(angle);
		float cosA = std::cos(angle);
		float u = float(i) / float(kCylinderDivide);

		// 下
		Vector3 posBottom = { cosA * kBottomRadius, 0.0f, sinA * kBottomRadius };
		Vector3 normal = { cosA, 0.0f, sinA };
		model->data_.vertices.push_back({ {posBottom.x, posBottom.y, posBottom.z, 1.0f}, {u, 1.0f}, normal });
		newModelMesh.vertices.push_back({ {posBottom.x, posBottom.y, posBottom.z, 1.0f}, {u, 1.0f}, normal });

		// 上
		Vector3 posTop = { cosA * kTopRadius, kHeight, sinA * kTopRadius };
		model->data_.vertices.push_back({ {posTop.x, posTop.y, posTop.z, 1.0f}, {u, 0.0f}, normal });
		newModelMesh.vertices.push_back({ {posTop.x, posTop.y, posTop.z, 1.0f}, {u, 0.0f}, normal });

	}

	// インデックス生成
	for (uint32_t i = 0; i < kCylinderDivide; i++) {
		uint32_t bottom0 = i * 2;
		uint32_t top0 = bottom0 + 1;
		uint32_t bottom1 = bottom0 + 2;
		uint32_t top1 = bottom0 + 3;

		// 三角形1
		model->data_.indicies.push_back(bottom0);
		model->data_.indicies.push_back(top0);
		model->data_.indicies.push_back(bottom1);

		newModelMesh.indicies.push_back(bottom0);
		newModelMesh.indicies.push_back(top0);
		newModelMesh.indicies.push_back(bottom1);

		// 三角形2
		model->data_.indicies.push_back(bottom1);
		model->data_.indicies.push_back(top0);
		model->data_.indicies.push_back(top1);

		newModelMesh.indicies.push_back(bottom1);
		newModelMesh.indicies.push_back(top0);
		newModelMesh.indicies.push_back(top1);
	}
	model->data_.meshes.push_back(newModelMesh);

	return model->data_;
}


void ModelManager::AddModel(const std::string& filename, Model* model) {
	std::unique_ptr<Model> newModel;
	newModel.reset(new Model(*model));
	models_.insert(std::make_pair(filename, std::move(newModel)));
}


void ModelManager::LoadModelFile(bool overWrite) {
#ifdef _DEBUG
	modelFileList.clear();

	if (!std::filesystem::exists(kDirectoryPath_)) return;

	for (const auto& entry : std::filesystem::directory_iterator(kDirectoryPath_)) {
		if (entry.is_regular_file()) {
			auto path = entry.path();
			if (path.extension() == ".obj") {
				if (overWrite) {
					modelFileList.push_back(std::make_pair(path.filename().string(), true));
				} else {
					modelFileList.push_back(std::make_pair(path.filename().string(), false));
				}
			}
		}
	}

#endif // _DEBUG
}

void ModelManager::SetModelFileOnceLoad(const std::string& name) {
	for (auto& pair : modelFileList) {
		if (pair.first == name) {
			pair.second = false;
			break; // 名前がユニークなら break でOK
		}
	}
}

void ModelManager::NormalCommand() {
	dxcommon_->GetDXCommand()->SetViewAndscissor();
	dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Normal);
	dxcommon_->GetDXCommand()->GetList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	lightManager_->SetLightCommand(dxcommon_->GetCommandList());
	PickingCommand();
}

void ModelManager::PickingUpdate() {
#ifdef _DEBUG
	if (!pickingData_ || !dxcommon_) return;

	ImGuiIO& io = ImGui::GetIO();

	Vector2 mousePos = { io.MousePos.x, io.MousePos.y };
	bool isMouseOnGUI = io.WantCaptureMouse || ImGuizmo::IsOver() || ImGuizmo::IsUsing();
	bool isMouseInWindow =
		(mousePos.x >= 0 && mousePos.y >= 0 &&
			mousePos.x < MyWin::kWindowWidth && mousePos.y < MyWin::kWindowHeight);

	// === ピッキング有効／無効の設定 ===
	if (isMouseOnGUI || !isMouseInWindow) {
		pickingData_->pickingEnable = 0;
		pickingData_->pickingPixelCoord[0] = -1;
		pickingData_->pickingPixelCoord[1] = -1;
	} else if (Input::GetInstance()->IsTriggerMouse(0)){
		isPicked_ = true;
		pickingData_->pickingEnable = 1;
		pickingData_->pickingPixelCoord[0] = int(mousePos.x);
		pickingData_->pickingPixelCoord[1] = int(mousePos.y);
	} else {
		pickingData_->pickingEnable = 0;
	}

	PickingBuffer* mapped = nullptr;
	D3D12_RANGE readRange{ 0, sizeof(PickingBuffer) }; // 読み取り範囲
	if (SUCCEEDED(pickingBufferReadBack_->Map(0, &readRange, reinterpret_cast<void**>(&mapped)))) {
		lastPicked_.objID = mapped->objID;
		lastPicked_.depth = mapped->depth;
		pickingBufferReadBack_->Unmap(0, nullptr);
	}

#endif // _DEBUG
}

void ModelManager::PickingCommand() {
	dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(7, pickBufferHandle_.second);
	dxcommon_->GetCommandList()->SetGraphicsRootConstantBufferView(8, pickingDataResource_->GetGPUVirtualAddress());
}

void ModelManager::PickingDataReset() {
#ifdef _DEBUG

	PickingBuffer init{};
	init.objID = lastPicked_.objID; // 前回の objID
	init.depth = 1.0f;              // Depth をリセット

	// マップしてデータを書き換える
	void* mapped = nullptr;
	D3D12_RANGE range = { 0, 0 }; // 読み取りはしない
	HRESULT hr = initUploadBuffer_->Map(0, &range, &mapped);
	assert(SUCCEEDED(hr));
	memcpy(mapped, &init, sizeof(PickingBuffer));
	initUploadBuffer_->Unmap(0, nullptr);

	// UAV にコピー
	dxcommon_->GetCommandList()->CopyResource(pickingBufferResource_.Get(), initUploadBuffer_.Get());

#endif // _DEBUG
}

void ModelManager::PickingDataCopy() {
	if (isOnce_) isOnce_ = false;
	if (isPicked_) {
		isPicked_ = false;
	}
	if (preObjId_ != lastPicked_.objID) {
		preObjId_ = lastPicked_.objID;
		isOnce_ = true;
	}

	dxcommon_->TransitionResource(pickingBufferResource_.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE);
	dxcommon_->GetCommandList()->CopyResource(pickingBufferReadBack_.Get(), pickingBufferResource_.Get());

}

MaterialDataPath ModelManager::LoadMaterialFile(const std::string& filename) {
	MaterialDataPath materialData;
	ModelManager* instance = GetInstance();
	std::string line;
	std::ifstream file(instance->kDirectoryPath_ + filename);
	assert(file.is_open());

	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		if (identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			materialData.textureFilePath = textureFilename;
		}
	}

	if (materialData.textureFilePath.empty()) {
		std::string whiteTexture = "white2x2.png";
		materialData.textureFilePath = whiteTexture;
	}

	return materialData;
}

Node ModelManager::ReadNode(aiNode* node) {
	Node result;
	aiVector3D scale, translate;
	aiQuaternion rotate;
	node->mTransformation.Decompose(scale, rotate, translate);
	result.transform.scale = { scale.x,scale.y,scale.z };
	result.transform.rotate = { rotate.x,-rotate.y,-rotate.z,rotate.w };
	result.transform.translate = { -translate.x,translate.y,translate.z };
	result.local = MakeAffineMatrix(result.transform.scale, result.transform.rotate, result.transform.translate);

	result.name = node->mName.C_Str();
	result.children.resize(node->mNumChildren);
	for (uint32_t i = 0; i < node->mNumChildren; i++) {
		result.children[i] = ReadNode(node->mChildren[i]);
	}

	return result;
}
