#include "ModelManager.h"
#include <fstream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


ModelManager::~ModelManager() {
}


ModelManager* ModelManager::GetInstance() {
	static ModelManager instance;
	return &instance;
}


void ModelManager::Initialize(DXCom* pDxcom, LightManager* pLight) {
	dxcommon_ = pDxcom;
	lightManager_ = pLight;
}


void ModelManager::Finalize() {
	dxcommon_ = nullptr;

	models_.clear();
}


void ModelManager::LoadOBJ(const std::string& filename) {
	ModelManager* instance = GetInstance();
	auto iterator = instance->models_.find(filename);
	if (iterator != instance->models_.end()) {
		return;
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
				model->data_.indicies.push_back(vertexIndex + meshVertexCount);
				newModelMesh.indicies.push_back(vertexIndex);
			}
		}
		meshVertexCount += mesh->mNumVertices;

		// === メッシュに対応するマテリアルを取得 ===
		uint32_t materialIndex = mesh->mMaterialIndex;
		aiMaterial* material = scene->mMaterials[materialIndex];

		aiString textureFileName;
		std::string texturePath;

		if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0 &&
			material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFileName) == AI_SUCCESS) {
			texturePath = textureFileName.C_Str();
		} else {
			texturePath = "white2x2.png";  // デフォルト
		}
		newModelMesh.material.textureFilePath = texturePath;

		model->data_.meshes.push_back(newModelMesh);
	}

	instance->models_.insert(std::make_pair(filename, std::move(model)));
}

void ModelManager::LoadGLTF(const std::string& filename) {
	ModelManager* instance = GetInstance();
	auto iterator = instance->models_.find(filename);
	if (iterator != instance->models_.end()) {
		return;
	}

	std::unique_ptr<Model> model;
	model.reset(new Model());

	Assimp::Importer importer;
	std::string path = instance->kDirectoryPath_ + "/" + filename;
	const aiScene* scene = importer.ReadFile(path.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs);
	assert(scene->HasMeshes());

	ModelMesh newModelMesh{};
	// Mesh解析
	uint32_t meshVertexCount = 0;
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++) {
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
				model->data_.indicies.push_back(vertexIndex + meshVertexCount);
				newModelMesh.indicies.push_back(vertexIndex);
			}
		}
		meshVertexCount += mesh->mNumVertices;

		// === メッシュに対応するマテリアルを取得 ===
		uint32_t materialIndex = mesh->mMaterialIndex;
		aiMaterial* material = scene->mMaterials[materialIndex];

		aiString textureFileName;
		std::string texturePath;

		if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0 &&
			material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFileName) == AI_SUCCESS) {
			texturePath = textureFileName.C_Str();
		} else {
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
				jointWeightData.vertexWeights.push_back({ bone->mWeights[weightIndex].mWeight,bone->mWeights[weightIndex].mVertexId });
			}
		}
	}
	model->data_.rootNode = ReadNode(scene->mRootNode);

	instance->models_.insert(std::make_pair(filename, std::move(model)));
}


ModelData ModelManager::FindModel(const std::string& filename) {
	ModelManager* instance = GetInstance();
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

void ModelManager::CreateRing(float out, float in, float radius) {
	ModelManager* instance = GetInstance();

	auto iterator = instance->models_.find("Ring");
	if (iterator != instance->models_.end()) {
		return;
	}

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

		// 外周
		model->data_.vertices.push_back({ {-sinA * kOuterRadius, cosA * kOuterRadius, 0.0f, 1.0f}, {u, 0.0f}, {0,0,1} });
		newModelMesh.vertices.push_back({ {-sinA * kOuterRadius, cosA * kOuterRadius, 0.0f, 1.0f}, {u, 0.0f}, {0,0,1} });
		// 内周
		model->data_.vertices.push_back({ {-sinA * kInnerRadius, cosA * kInnerRadius, 0.0f, 1.0f}, {u, 1.0f}, {0,0,1} });
		newModelMesh.vertices.push_back({ {-sinA * kInnerRadius, cosA * kInnerRadius, 0.0f, 1.0f}, {u, 1.0f}, {0,0,1} });
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

	instance->models_.insert(std::make_pair("Ring", std::move(model)));
}

void ModelManager::CreateSylinder(float topRadius, float bottomRadius, float height) {
	ModelManager* instance = GetInstance();

	auto iterator = instance->models_.find("Cylinder");
	if (iterator != instance->models_.end()) {
		return;
	}

	std::unique_ptr<Model> model;
	model.reset(new Model());
	ModelMesh newModelMesh{};

	const uint32_t kCylinderDivide = 32;
	const float kTopRadius = topRadius;
	const float kBottomRadius = bottomRadius;
	const float kHeight = height;
	const float halfHeight = kHeight / 2.0f;
	const float radianPerDivide = 2.0f * std::numbers::pi_v<float> / float(kCylinderDivide);

	for (uint32_t i = 0; i <= kCylinderDivide; i++) {
		float angle = i * radianPerDivide;
		float sinA = std::sin(angle);
		float cosA = std::cos(angle);
		float u = float(i) / float(kCylinderDivide);

		// 下
		Vector3 posBottom = { cosA * kBottomRadius, -halfHeight, sinA * kBottomRadius };
		Vector3 normal = { cosA, 0.0f, sinA };
		model->data_.vertices.push_back({ {posBottom.x, posBottom.y, posBottom.z, 1.0f}, {u, 1.0f}, normal });
		newModelMesh.vertices.push_back({ {posBottom.x, posBottom.y, posBottom.z, 1.0f}, {u, 1.0f}, normal });

		// 上
		Vector3 posTop = { cosA * kTopRadius, +halfHeight, sinA * kTopRadius };
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

	instance->models_.insert(std::make_pair("Cylinder", std::move(model)));
}


void ModelManager::AddModel(const std::string& filename, Model* model) {
	std::unique_ptr<Model> newModel;
	newModel.reset(new Model(*model));
	models_.insert(std::make_pair(filename, std::move(newModel)));
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
