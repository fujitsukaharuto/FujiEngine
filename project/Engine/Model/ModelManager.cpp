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


void ModelManager::Initialize() {
}


void ModelManager::Finalize() {
	ModelManager* instance = GetInstance();
	instance->models_.clear();
}


void ModelManager::LoadOBJ(const std::string& filename) {
	ModelManager* instance = GetInstance();

	auto iterator = instance->models_.find(filename);
	if (iterator != instance->models_.end()) {
		return;
	}


	std::unique_ptr<Model> model;
	model.reset(new Model());
	Mesh newMesh{};
	Material newMaterial{};


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

			newMesh.AddVertex({ {vertex.position},{vertex.texcoord},{vertex.normal} });
		}

		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++) {
			aiFace& face = mesh->mFaces[faceIndex];
			assert(face.mNumIndices == 3);

			for (uint32_t element = 0; element < face.mNumIndices; element++) {
				uint32_t vertexIndex = face.mIndices[element];
				newMesh.AddIndex(vertexIndex + meshVertexCount);
			}
		}
		meshVertexCount += mesh->mNumVertices;
	}

	// Material解析
	for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; materialIndex++) {

		aiMaterial* material = scene->mMaterials[materialIndex];

		if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0) {

			aiString textureFileName;
			material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFileName);
			newMaterial.SetTextureNamePath((textureFileName).C_Str());
			newMaterial.CreateMaterial();
			model->AddMaterial(newMaterial);
			model->SetTextureName((textureFileName).C_Str());
		}
	}

	if (model->GetTextuerName().empty()) {
		const std::string defaultTexture = "white2x2.png";
		newMaterial.SetTextureNamePath(defaultTexture);
		newMaterial.CreateMaterial();
		model->AddMaterial(newMaterial);
		model->SetTextureName(defaultTexture);
	}


	newMesh.CreateMesh();

	model->AddMesh(newMesh);

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
	Mesh newMesh{};
	Material newMaterial{};


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

			newMesh.AddVertex({ {vertex.position},{vertex.texcoord},{vertex.normal} });
			model->data_.vertices.push_back({ {vertex.position},{vertex.texcoord},{vertex.normal} });
		}

		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++) {
			aiFace& face = mesh->mFaces[faceIndex];
			assert(face.mNumIndices == 3);
			
			for (uint32_t element = 0; element < face.mNumIndices; element++) {
				uint32_t vertexIndex = face.mIndices[element];
				newMesh.AddIndex(vertexIndex);
				model->data_.indicies.push_back(vertexIndex + meshVertexCount);
			}
		}
		meshVertexCount += mesh->mNumVertices;

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

	// Material解析
	for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; materialIndex++) {

		aiMaterial* material = scene->mMaterials[materialIndex];

		if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0) {

			aiString textureFileName;
			material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFileName);
			newMaterial.SetTextureNamePath((textureFileName).C_Str());
			newMaterial.CreateMaterial();
			model->AddMaterial(newMaterial);
			model->SetTextureName((textureFileName).C_Str());
		}
	}

	if (model->GetTextuerName().empty()) {
		const std::string defaultTexture = "white2x2.png";
		newMaterial.SetTextureNamePath(defaultTexture);
		newMaterial.CreateMaterial();
		model->AddMaterial(newMaterial);
		model->SetTextureName(defaultTexture);
	}


	newMesh.CreateMesh();

	model->AddMesh(newMesh);
	model->data_.rootNode = ReadNode(scene->mRootNode);

	instance->models_.insert(std::make_pair(filename, std::move(model)));
}


Model* ModelManager::FindModel(const std::string& filename) {
	ModelManager* instance = GetInstance();
	auto iterator = instance->models_.find(filename);
	if (iterator != instance->models_.end()) {
		return iterator->second.get();
	}
	return nullptr;
}


void ModelManager::CreateSphere() {
	ModelManager* instance = GetInstance();

	auto iterator = instance->models_.find("Sphere");
	if (iterator != instance->models_.end()) {
		return;
	}


	std::unique_ptr<Model> model;
	model.reset(new Model());
	Mesh mesh{};
	Material material{};


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

			mesh.AddVertex({
				{x, y, z, 1.0f},   // Position
				{u, v},            // UV
				{x, y, z}          // Normal（球の中心を原点とした法線）
				});
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

			// 三角形1
			mesh.AddIndex(v0);
			mesh.AddIndex(v2);
			mesh.AddIndex(v1);

			// 三角形2
			mesh.AddIndex(v1);
			mesh.AddIndex(v2);
			mesh.AddIndex(v3);
		}
	}


	mesh.CreateMesh();
	material.CreateMaterial();


	model->AddMesh(mesh);
	model->AddMaterial(material);

	instance->models_.insert(std::make_pair("Sphere", std::move(model)));
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
