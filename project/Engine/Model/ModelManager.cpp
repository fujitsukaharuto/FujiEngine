#include "ModelManager.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Engine/DX/DX12Helper.h"
#include "Engine/DX/SRVManager.h"
#include "Engine/WinApp/MyWindow.h"
#include "Engine/ImGuiManager/ImGuiManager.h"
#include "Engine/Input/Input.h"
#include "Engine/Light/LightManager.h"
#ifdef _DEBUGMODE
#include "ImGuizmo.h"
#endif // _DEBUG

using namespace Core;
using namespace Graphics;
using namespace Math;


ModelManager::~ModelManager() {
}


ModelManager* ModelManager::GetInstance() {
	static ModelManager instance;
	return &instance;
}


void ModelManager::Initialize(DXCom* pDxcom, Graphics::LightManager* pLight) {
	dxcommon_ = pDxcom;
	lightManager_ = pLight;
	LoadModelFile();

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		pickingBufferResource_[i] = DXC::Helper::CreateUAVResource(dxcommon_->GetDevice(), sizeof(PickingBuffer));

		uint32_t srvIndex = SRVManager::GetInstance()->Allocate();
		pickBufferHandle_[i].first = SRVManager::GetInstance()->GetCPUDescriptorHandle(srvIndex);
		pickBufferHandle_[i].second = SRVManager::GetInstance()->GetGPUDescriptorHandle(srvIndex);
		SRVManager::GetInstance()->CreateStructuredUAV(srvIndex, pickingBufferResource_[i].Get(),
			static_cast<UINT>(1), static_cast<UINT>(sizeof(PickingBuffer)));

		pickingBufferReadBack_[i] = DXC::Helper::CreateReadbackResource(dxcommon_->GetDevice(), sizeof(PickingBuffer));


		pickingDataResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(PickingData));
		pickingDataGPU_[i] = nullptr;
		pickingDataResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&pickingDataGPU_[i]));
	}

	pickingData_.pickingEnable = 0;
	pickingData_.pickingPixelCoord[0] = -1;
	pickingData_.pickingPixelCoord[1] = -1;

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		CopyData(i);
	}

	lastPicked_.objID = -1;
	lastPicked_.depth = 1.0f;
	PickingBuffer init{};
	init.objID = -1;
	init.depth = 1.0f;

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		initUploadBuffer_[i] = DXC::Helper::CreateUploadBuffer(dxcommon_->GetDevice(), sizeof(PickingBuffer), &init);

		dxcommon_->GetImmediateList()->CopyResource(pickingBufferResource_[i].Get(), initUploadBuffer_[i].Get());
	}
	dxcommon_->CommandExecution();
}


void ModelManager::Finalize() {
	dxcommon_ = nullptr;

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		pickingBufferResource_[i].Reset();
		pickingBufferReadBack_[i].Reset();
		pickingDataResource_[i].Reset();
		initUploadBuffer_[i].Reset();
	}

	modelFileList.clear();
	models_.clear();
}


void ModelManager::LoadModelByExtension(const std::string& filename, bool overWrite) {
	std::filesystem::path path(filename);
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
	model = std::make_unique<Model>();

	Assimp::Importer importer;
	std::string path = instance->kDirectoryPath_ + filename;
	const aiScene* scene = importer.ReadFile(path.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
	assert(scene->HasMeshes());

	// Mesh解析
	uint32_t meshVertexCount = 0;
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++) {
		aiMesh* mesh = scene->mMeshes[meshIndex];
		assert(mesh->HasNormals());
		bool hasTangent = mesh->HasTangentsAndBitangents();
		bool hasTexcoord = mesh->HasTextureCoords(0);

		ModelMesh newModelMesh{};
		// Vertex解析
		for (uint32_t element = 0; element < mesh->mNumVertices; element++) {

			aiVector3D& position = mesh->mVertices[element];
			aiVector3D& normal = mesh->mNormals[element];

			VertexData vertex;
			vertex.pos = { position.x,position.y,position.z,1.0f };
			vertex.normal = { normal.x,normal.y,normal.z };

			if (hasTangent) {
				aiVector3D& tangent = mesh->mTangents[element];
				vertex.tangent = { tangent.x,tangent.y,tangent.z };
			} else {
				vertex.tangent = { 1.0f,0.0f,0.0f };
			}

			if (hasTexcoord) {
				aiVector3D& texcoord = mesh->mTextureCoords[0][element];
				vertex.uv = { texcoord.x,texcoord.y };
			} else {
				vertex.uv = { 0.0f,0.0f };
			}

			vertex.pos.x *= -1.0f;
			vertex.normal.x *= -1.0f;
			vertex.tangent.x *= -1.0f;

			model->GetModelData().vertices.push_back({ {vertex.pos},{vertex.uv},{vertex.normal},{vertex.tangent} });
			newModelMesh.vertices.push_back({ {vertex.pos},{vertex.uv},{vertex.normal},{vertex.tangent} });
		}

		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++) {
			aiFace& face = mesh->mFaces[faceIndex];
			assert(face.mNumIndices == 3);

			for (uint32_t element = 0; element < face.mNumIndices; element++) {
				uint32_t vertexIndex = face.mIndices[element];
				model->GetModelData().indicies.push_back(vertexIndex + meshVertexCount);
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

		aiString normalMapFileName;
		std::string normalMapPath;

		if (material->GetTextureCount(aiTextureType_NORMALS) > 0 &&
			material->GetTexture(aiTextureType_NORMALS, 0, &normalMapFileName) == AI_SUCCESS) {
			normalMapPath = normalMapFileName.C_Str();
		} else if (material->GetTextureCount(aiTextureType_HEIGHT) > 0 &&
			material->GetTexture(aiTextureType_HEIGHT, 0, &normalMapFileName) == AI_SUCCESS) {
			normalMapPath = normalMapFileName.C_Str();
		} else {
			normalMapPath = "defaultNormal.png";
		}
		newModelMesh.material.normalFilePath = normalMapPath;

		model->GetModelData().meshes.push_back(newModelMesh);
	}

	for (size_t i = 0; i < model->GetModelData().meshes.size(); i++) {
		Mesh newMesh{};
		for (size_t index = 0; index < model->GetModelData().meshes[i].vertices.size(); index++) {
			VertexData newVertex = model->GetModelData().meshes[i].vertices[index];
			newMesh.AddVertex({ { newVertex.pos },{newVertex.uv},{newVertex.normal},{newVertex.tangent} });
		}
		for (size_t index = 0; index < model->GetModelData().meshes[i].indicies.size(); index++) {
			uint32_t newIndex = model->GetModelData().meshes[i].indicies[index];
			newMesh.AddIndex(newIndex);
		}
		newMesh.CreateMesh();
		model->AddMesh(std::move(newMesh));
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
	model = std::make_unique<Model>();

	Assimp::Importer importer;
	std::string path = instance->kDirectoryPath_ + "/" + filename;
	const aiScene* scene = importer.ReadFile(path.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
	assert(scene->HasMeshes());

	// Mesh解析
	uint32_t meshVertexCount = 0;
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++) {
		ModelMesh newModelMesh{};
		aiMesh* mesh = scene->mMeshes[meshIndex];
		assert(mesh->HasNormals());
		bool hasTangent = mesh->HasTangentsAndBitangents();
		bool hasTexcoord = mesh->HasTextureCoords(0);

		// Vertex解析
		for (uint32_t element = 0; element < mesh->mNumVertices; element++) {
			aiVector3D& position = mesh->mVertices[element];
			aiVector3D& normal = mesh->mNormals[element];

			VertexData vertex;
			vertex.pos = { position.x,position.y,position.z,1.0f };
			vertex.normal = { normal.x,normal.y,normal.z };

			if (hasTangent) {
				aiVector3D& tangent = mesh->mTangents[element];
				vertex.tangent = { tangent.x,tangent.y,tangent.z };
			} else {
				vertex.tangent = { 1.0f,0.0f,0.0f };
			}

			if (hasTexcoord) {
				aiVector3D& texcoord = mesh->mTextureCoords[0][element];
				vertex.uv = { texcoord.x,texcoord.y };
			} else {
				vertex.uv = { 0.0f,0.0f };
			}

			vertex.pos.x *= -1.0f;
			vertex.normal.x *= -1.0f;
			vertex.tangent.x *= -1.0f;

			model->GetModelData().vertices.push_back({ {vertex.pos},{vertex.uv},{vertex.normal},{vertex.tangent} });
			newModelMesh.vertices.push_back({ {vertex.pos},{vertex.uv},{vertex.normal},{vertex.tangent} });
		}

		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++) {
			aiFace& face = mesh->mFaces[faceIndex];
			assert(face.mNumIndices == 3);

			for (uint32_t element = 0; element < face.mNumIndices; element++) {
				uint32_t vertexIndex = face.mIndices[element];
				model->GetModelData().indicies.push_back(vertexIndex + meshVertexCount);
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
		
		aiString normalMapFileName;
		std::string normalMapPath;

		if (material->GetTextureCount(aiTextureType_NORMALS) > 0 &&
			material->GetTexture(aiTextureType_NORMALS, 0, &normalMapFileName) == AI_SUCCESS) {
			normalMapPath = normalMapFileName.C_Str();
		} else if (material->GetTextureCount(aiTextureType_HEIGHT) > 0 &&
			material->GetTexture(aiTextureType_HEIGHT, 0, &normalMapFileName) == AI_SUCCESS) {
			normalMapPath = normalMapFileName.C_Str();
		} else {
			normalMapPath = "defaultNormal.png";
		}
		newModelMesh.material.normalFilePath = normalMapPath;
		
		model->GetModelData().meshes.push_back(newModelMesh);

		// SkinCluster構築用のデータ取得
		for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++) {
			aiBone* bone = mesh->mBones[boneIndex];
			std::string jointName = bone->mName.C_Str();
			JointWeightData& jointWeightData = model->GetModelData().skinClusterData[jointName];

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
	model->GetModelData().rootNode = ReadNode(scene->mRootNode);

	for (size_t i = 0; i < model->GetModelData().meshes.size(); i++) {
		Mesh newMesh{};
		for (size_t index = 0; index < model->GetModelData().meshes[i].vertices.size(); index++) {
			VertexData newVertex = model->GetModelData().meshes[i].vertices[index];
			newMesh.AddVertex({ { newVertex.pos },{newVertex.uv},{newVertex.normal},{newVertex.tangent} });
		}
		for (size_t index = 0; index < model->GetModelData().meshes[i].indicies.size(); index++) {
			uint32_t newIndex = model->GetModelData().meshes[i].indicies[index];
			newMesh.AddIndex(newIndex);
		}
		newMesh.CreateMesh();
		model->AddMesh(std::move(newMesh));
	}

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
		return iterator->second->GetModelData();
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
	model = std::make_unique<Model>();
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
			float u = 1.0f - float(lonIndex) / float(kSubdivision); // 経度でuを算出（0〜1）を反転

			float x = cosf(lat) * cosf(lon);
			float y = sinf(lat);
			float z = cosf(lat) * sinf(lon);

			Vector3 normal = { x, y, z };
			Vector3 tangent = { cosf(lat) * sinf(lon), 0.0f, -cosf(lat) * cosf(lon) };

			if (Vector3::Length(tangent) < 0.0001f) {
				tangent = { 1.0f, 0.0f, 0.0f };
			} else {
				tangent = Vector3::Normalize(tangent);
			}

			// X軸を反転させて、LoadOBJ等との整合性を保つ
			x *= -1.0f;
			normal.x *= -1.0f;
			tangent.x *= -1.0f;

			model->GetModelData().vertices.push_back({ {x, y, z, 1.0f},{u, v},normal,tangent });
			newModelMesh.vertices.push_back({ {x, y, z, 1.0f},{u, v},normal,tangent });
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

			// 表側を向くように順序を修正 (v0, v2, v1) (v1, v2, v3)
			model->GetModelData().indicies.push_back(v0);
			newModelMesh.indicies.push_back(v0);
			model->GetModelData().indicies.push_back(v2);
			newModelMesh.indicies.push_back(v2);
			model->GetModelData().indicies.push_back(v1);
			newModelMesh.indicies.push_back(v1);

			model->GetModelData().indicies.push_back(v1);
			newModelMesh.indicies.push_back(v1);
			model->GetModelData().indicies.push_back(v2);
			newModelMesh.indicies.push_back(v2);
			model->GetModelData().indicies.push_back(v3);
			newModelMesh.indicies.push_back(v3);
		}
	}
	model->GetModelData().meshes.push_back(newModelMesh);

	for (size_t i = 0; i < model->GetModelData().meshes.size(); i++) {
		Mesh newMesh{};
		for (size_t index = 0; index < model->GetModelData().meshes[i].vertices.size(); index++) {
			VertexData newVertex = model->GetModelData().meshes[i].vertices[index];
			newMesh.AddVertex({ { newVertex.pos },{newVertex.uv},{newVertex.normal},{newVertex.tangent} });
		}
		for (size_t index = 0; index < model->GetModelData().meshes[i].indicies.size(); index++) {
			uint32_t newIndex = model->GetModelData().meshes[i].indicies[index];
			newMesh.AddIndex(newIndex);
		}
		newMesh.CreateMesh();
		model->AddMesh(std::move(newMesh));
	}

	instance->models_.insert(std::make_pair("Sphere", std::move(model)));
}

ModelData ModelManager::CreateRing(float out, float in, float radius, bool horizon) {
	std::string key = "Ring_" + std::to_string(out) + "_" + std::to_string(in) + "_" + std::to_string(radius) + "_" + std::to_string(horizon);
	ModelManager* instance = GetInstance();
	auto iterator = instance->models_.find(key);
	if (iterator != instance->models_.end()) {
		return instance->models_[key]->GetModelData();
	}

	std::unique_ptr<Model> model;
	model = std::make_unique<Model>();
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

		Vector3 normal = { 0,0,1 };
		Vector3 tangent = { cosA, 0.0f, sinA };

		if (horizon) {
			// 外周
			model->GetModelData().vertices.push_back({ {-sinA * kOuterRadius, 0.0f, cosA * kOuterRadius, 1.0f}, {u, 0.0f}, normal, tangent });
			newModelMesh.vertices.push_back({ {-sinA * kOuterRadius, 0.0f, cosA * kOuterRadius, 1.0f}, {u, 0.0f}, normal, tangent });
			// 内周
			model->GetModelData().vertices.push_back({ {-sinA * kInnerRadius, 0.0f, cosA * kInnerRadius, 1.0f}, {u, 1.0f}, normal, tangent });
			newModelMesh.vertices.push_back({ {-sinA * kInnerRadius, 0.0f, cosA * kInnerRadius, 1.0f}, {u, 1.0f}, normal, tangent });
		} else {
			// 外周
			model->GetModelData().vertices.push_back({ {-sinA * kOuterRadius, cosA * kOuterRadius, 0.0f, 1.0f}, {u, 0.0f}, normal, tangent });
			newModelMesh.vertices.push_back({ {-sinA * kOuterRadius, cosA * kOuterRadius, 0.0f, 1.0f}, {u, 0.0f}, normal, tangent });
			// 内周
			model->GetModelData().vertices.push_back({ {-sinA * kInnerRadius, cosA * kInnerRadius, 0.0f, 1.0f}, {u, 1.0f}, normal, tangent });
			newModelMesh.vertices.push_back({ {-sinA * kInnerRadius, cosA * kInnerRadius, 0.0f, 1.0f}, {u, 1.0f}, normal, tangent });
		}
	}

	// インデックス生成
	for (uint32_t i = 0; i < kRingDivide; i++) {
		uint32_t outer0 = i * 2;
		uint32_t inner0 = outer0 + 1;
		uint32_t outer1 = outer0 + 2;
		uint32_t inner1 = outer0 + 3;

		// 三角形1
		model->GetModelData().indicies.push_back(outer0);
		newModelMesh.indicies.push_back(outer0);
		model->GetModelData().indicies.push_back(inner0);
		newModelMesh.indicies.push_back(inner0);
		model->GetModelData().indicies.push_back(outer1);
		newModelMesh.indicies.push_back(outer1);

		// 三角形2
		model->GetModelData().indicies.push_back(outer1);
		newModelMesh.indicies.push_back(outer1);
		model->GetModelData().indicies.push_back(inner0);
		newModelMesh.indicies.push_back(inner0);
		model->GetModelData().indicies.push_back(inner1);
		newModelMesh.indicies.push_back(inner1);
	}
	model->GetModelData().meshes.push_back(newModelMesh);

	for (size_t i = 0; i < model->GetModelData().meshes.size(); i++) {
		Mesh newMesh{};
		for (size_t index = 0; index < model->GetModelData().meshes[i].vertices.size(); index++) {
			VertexData newVertex = model->GetModelData().meshes[i].vertices[index];
			newMesh.AddVertex({ { newVertex.pos },{newVertex.uv},{newVertex.normal},{newVertex.tangent} });
		}
		for (size_t index = 0; index < model->GetModelData().meshes[i].indicies.size(); index++) {
			uint32_t newIndex = model->GetModelData().meshes[i].indicies[index];
			newMesh.AddIndex(newIndex);
		}
		newMesh.CreateMesh();
		model->AddMesh(std::move(newMesh));
	}

	instance->models_.insert(std::make_pair(key, std::move(model)));

	return instance->models_[key]->GetModelData();
}

ModelData ModelManager::CreateCylinder(float topRadius, float bottomRadius, float height) {
	std::string key = "Cylinder_" + std::to_string(topRadius) + "_" + std::to_string(bottomRadius) + "_" + std::to_string(height);
	ModelManager* instance = GetInstance();
	auto iterator = instance->models_.find(key);
	if (iterator != instance->models_.end()) {
		return instance->models_[key]->GetModelData();
	}

	std::unique_ptr<Model> model;
	model = std::make_unique<Model>();
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
		model->GetModelData().vertices.push_back({ {posBottom.x, posBottom.y, posBottom.z, 1.0f}, {u, 1.0f}, normal });
		newModelMesh.vertices.push_back({ {posBottom.x, posBottom.y, posBottom.z, 1.0f}, {u, 1.0f}, normal });

		// 上
		Vector3 posTop = { cosA * kTopRadius, kHeight, sinA * kTopRadius };
		model->GetModelData().vertices.push_back({ {posTop.x, posTop.y, posTop.z, 1.0f}, {u, 0.0f}, normal });
		newModelMesh.vertices.push_back({ {posTop.x, posTop.y, posTop.z, 1.0f}, {u, 0.0f}, normal });

	}

	// インデックス生成
	for (uint32_t i = 0; i < kCylinderDivide; i++) {
		uint32_t bottom0 = i * 2;
		uint32_t top0 = bottom0 + 1;
		uint32_t bottom1 = bottom0 + 2;
		uint32_t top1 = bottom0 + 3;

		// 三角形1
		model->GetModelData().indicies.push_back(bottom0);
		model->GetModelData().indicies.push_back(top0);
		model->GetModelData().indicies.push_back(bottom1);

		newModelMesh.indicies.push_back(bottom0);
		newModelMesh.indicies.push_back(top0);
		newModelMesh.indicies.push_back(bottom1);

		// 三角形2
		model->GetModelData().indicies.push_back(bottom1);
		model->GetModelData().indicies.push_back(top0);
		model->GetModelData().indicies.push_back(top1);

		newModelMesh.indicies.push_back(bottom1);
		newModelMesh.indicies.push_back(top0);
		newModelMesh.indicies.push_back(top1);
	}
	model->GetModelData().meshes.push_back(newModelMesh);

	for (size_t i = 0; i < model->GetModelData().meshes.size(); i++) {
		Mesh newMesh{};
		for (size_t index = 0; index < model->GetModelData().meshes[i].vertices.size(); index++) {
			VertexData newVertex = model->GetModelData().meshes[i].vertices[index];
			newMesh.AddVertex({ { newVertex.pos },{newVertex.uv},{newVertex.normal},{newVertex.tangent} });
		}
		for (size_t index = 0; index < model->GetModelData().meshes[i].indicies.size(); index++) {
			uint32_t newIndex = model->GetModelData().meshes[i].indicies[index];
			newMesh.AddIndex(newIndex);
		}
		newMesh.CreateMesh();
		model->AddMesh(std::move(newMesh));
	}

	instance->models_.insert(std::make_pair(key, std::move(model)));

	return instance->models_[key]->GetModelData();
}

void ModelManager::LoadModelFile(bool overWrite) {
#ifdef _DEBUGMODE
	modelFileList.clear();

	if (!std::filesystem::exists(kDirectoryPath_)) return;

	for (const auto& entry : std::filesystem::directory_iterator(kDirectoryPath_)) {
		if (entry.is_regular_file()) {
			auto path = entry.path();
			if (path.extension() == ".obj" || path.extension() == ".gltf") {
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

void Graphics::ModelManager::LoadAllFileData() {
	for (auto& pair : modelFileList) {
		LoadModelByExtension(pair.first);
	}
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
	dxcommon_->GetDXCommand()->SetViewAndScissor(MyWin::kWindowWidth, MyWin::kWindowHeight);
	dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Normal);
	dxcommon_->GetDXCommand()->GetList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	lightManager_->SetLightCommand(dxcommon_->GetCommandList());
	PickingCommand();
}

Model* ModelManager::GetModel(const std::string& filePath, bool overWrite) {
	ModelManager* instance = GetInstance();
	// すでにロード済みならそれを返す
	if (instance->models_.contains(filePath) && !overWrite) {
		return instance->models_[filePath].get();
	}
	// 無ければ新規作成
	LoadModelByExtension(filePath, overWrite);
	return instance->models_[filePath].get();
}

Model* ModelManager::GetSphereModel() {
	ModelManager* instance = GetInstance();
	// すでにロード済みならそれを返す
	if (instance->models_.contains("Sphere")) {
		return instance->models_["Sphere"].get();
	}
	// 無ければ新規作成
	CreateSphere();
	return instance->models_["Sphere"].get();
}

Model* ModelManager::GetRingModel(float out, float in, float radius, bool horizon) {
	ModelManager* instance = GetInstance();
	// 引数からユニークな名前（キー）を作る
	std::string key = "Ring_" + std::to_string(out) + "_" + std::to_string(in) + "_" + std::to_string(radius) + "_" + std::to_string(horizon);
	if (instance->models_.contains(key)) {
		return instance->models_[key].get();
	}
	// 無ければ新規作成
	CreateRing(out, in, radius, horizon);
	return instance->models_[key].get();
}

Model* ModelManager::GetCylinderModel(float topRadius, float bottomRadius, float height) {
	ModelManager* instance = GetInstance();
	// 引数からユニークな名前（キー）を作る
	std::string key = "Cylinder_" + std::to_string(topRadius) + "_" + std::to_string(bottomRadius) + "_" + std::to_string(height);
	if (instance->models_.contains(key)) {
		return instance->models_[key].get();
	}
	// 無ければ新規作成
	CreateCylinder(topRadius, bottomRadius, height);
	return instance->models_[key].get();
}

void ModelManager::PickingUpdate() {
#ifdef _DEBUGMODE
	if (!pickingDataGPU_ || !dxcommon_) return;

	ImGuiIO& io = ImGui::GetIO();

	Vector2 mousePos = { io.MousePos.x, io.MousePos.y };
	bool isMouseOnGUI = io.WantCaptureMouse || ImGuizmo::IsOver() || ImGuizmo::IsUsing();
	bool isMouseInWindow =
		(mousePos.x >= 0 && mousePos.y >= 0 &&
			mousePos.x < MyWin::kWindowWidth && mousePos.y < MyWin::kWindowHeight);

	// === ピッキング有効／無効の設定 ===
	if (isMouseOnGUI || !isMouseInWindow) {
		pickingData_.pickingEnable = 0;
		pickingData_.pickingPixelCoord[0] = -1;
		pickingData_.pickingPixelCoord[1] = -1;
	} else if (Input::GetInstance()->IsTriggerMouse(0)) {
		isPicked_ = true;
		pickingData_.pickingEnable = 1;
		pickingData_.pickingPixelCoord[0] = int(mousePos.x);
		pickingData_.pickingPixelCoord[1] = int(mousePos.y);
	} else {
		pickingData_.pickingEnable = 0;
	}

	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	CopyData(frameIndex);

	uint32_t readIndex =
		(frameIndex + DXC::kFrameCount_ - 1) % DXC::kFrameCount_;// 1フレーム前を読むこと
	PickingBuffer* mapped = nullptr;
	D3D12_RANGE readRange{ 0, sizeof(PickingBuffer) }; // 読み取り範囲
	if (SUCCEEDED(pickingBufferReadBack_[readIndex]->Map(0, &readRange, reinterpret_cast<void**>(&mapped)))) {
		lastPicked_.objID = mapped->objID;
		lastPicked_.depth = mapped->depth;
		pickingBufferReadBack_[readIndex]->Unmap(0, nullptr);
	}

#endif // _DEBUG
}

void ModelManager::PickingCommand() {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();

	PipelineManager::GetInstance()->SetGraphicsRootCBV(cList, "PickingData", pickingDataResource_[frameIndex]->GetGPUVirtualAddress());
	PipelineManager::GetInstance()->SetGraphicsRootDescriptorTable(cList, "gPickingBuffer", pickBufferHandle_[frameIndex].second);
}

void ModelManager::PickingDataReset() {
#ifdef _DEBUGMODE

	PickingBuffer init{};
	init.objID = lastPicked_.objID; // 前回の objID
	init.depth = 1.0f;              // Depth をリセット

	uint32_t frameIndex = dxcommon_->GetNowFrameCount();

	// マップしてデータを書き換える
	void* mapped = nullptr;
	D3D12_RANGE range = { 0, 0 }; // 読み取りはしない
	HRESULT hr = initUploadBuffer_[frameIndex]->Map(0, &range, &mapped);
	assert(SUCCEEDED(hr));
	memcpy(mapped, &init, sizeof(PickingBuffer));
	initUploadBuffer_[frameIndex]->Unmap(0, nullptr);

	// UAV にコピー
	dxcommon_->GetCommandList()->CopyResource(pickingBufferResource_[frameIndex].Get(), initUploadBuffer_[frameIndex].Get());

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
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();

	dxcommon_->TransitionResource(pickingBufferResource_[frameIndex].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE);
	dxcommon_->GetCommandList()->CopyResource(pickingBufferReadBack_[frameIndex].Get(), pickingBufferResource_[frameIndex].Get());

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

void ModelManager::CopyData(uint32_t frameIndex) {
	pickingDataGPU_[frameIndex]->pickingEnable = pickingData_.pickingEnable;
	pickingDataGPU_[frameIndex]->pickingPixelCoord[0] = pickingData_.pickingPixelCoord[0];
	pickingDataGPU_[frameIndex]->pickingPixelCoord[1] = pickingData_.pickingPixelCoord[1];
}
