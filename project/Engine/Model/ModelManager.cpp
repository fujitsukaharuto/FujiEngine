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
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++) {

		aiMesh* mesh = scene->mMeshes[meshIndex];
		assert(mesh->HasNormals());
		bool hasTexcoord = mesh->HasTextureCoords(0);


		// Face解析
		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++) {

			aiFace& face = mesh->mFaces[faceIndex];
			assert(face.mNumIndices == 3);

			// Vertex解析
			for (uint32_t element = 0; element < face.mNumIndices; element++) {

				uint32_t vertexIndex = face.mIndices[element];
				aiVector3D& position = mesh->mVertices[vertexIndex];
				aiVector3D& normal = mesh->mNormals[vertexIndex];

				VertexDate vertex;
				vertex.position = { position.x,position.y,position.z,1.0f };
				vertex.normal = { normal.x,normal.y,normal.z };

				if (hasTexcoord) {
					aiVector3D& texcoord = mesh->mTextureCoords[0][vertexIndex];
					vertex.texcoord = { texcoord.x,texcoord.y };
				}
				else {
					vertex.texcoord = { 0.0f,0.0f };
				}


				vertex.position.x *= -1.0f;
				vertex.normal.x *= -1.0f;

				newMesh.AddVertex({ {vertex.position},{vertex.texcoord},{vertex.normal} });

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
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++) {

		aiMesh* mesh = scene->mMeshes[meshIndex];
		assert(mesh->HasNormals());
		bool hasTexcoord = mesh->HasTextureCoords(0);


		// Face解析
		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++) {

			aiFace& face = mesh->mFaces[faceIndex];
			assert(face.mNumIndices == 3);

			// Vertex解析
			for (uint32_t element = 0; element < face.mNumIndices; element++) {

				uint32_t vertexIndex = face.mIndices[element];
				aiVector3D& position = mesh->mVertices[vertexIndex];
				aiVector3D& normal = mesh->mNormals[vertexIndex];

				VertexDate vertex;
				vertex.position = { position.x,position.y,position.z,1.0f };
				vertex.normal = { normal.x,normal.y,normal.z };

				if (hasTexcoord) {
					aiVector3D& texcoord = mesh->mTextureCoords[0][vertexIndex];
					vertex.texcoord = { texcoord.x,texcoord.y };
				}
				else {
					vertex.texcoord = { 0.0f,0.0f };
				}


				vertex.position.x *= -1.0f;
				vertex.normal.x *= -1.0f;

				newMesh.AddVertex({ {vertex.position},{vertex.texcoord},{vertex.normal} });

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
	const float kLonEvery = (pi * 2.0f) / static_cast<float>(kSubdivision);
	const float kLatEvery = (pi) / static_cast<float>(kSubdivision);

	for (uint32_t latIndex = 0; latIndex < kSubdivision; latIndex++) {
		float lat = -pi / 2.0f + kLatEvery * latIndex;
		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; lonIndex++) {
			float lon = lonIndex * kLonEvery;

			float u = float(lonIndex) / float(kSubdivision);
			float v = 1.0f - float(latIndex) / float(kSubdivision);


			mesh.AddVertex({ {(cosf(lat) * cosf(lon)), (sinf(lat)), (cosf(lat) * sinf(lon)), 1.0f},
				{u, v},
				{(cosf(lat) * cosf(lon)), (sinf(lat)), (cosf(lat) * sinf(lon))} });


			mesh.AddVertex({ {(cosf(lat + kLatEvery) * cosf(lon)), (sinf(lat + kLatEvery)), (cosf(lat + kLatEvery) * sinf(lon)), 1.0f},
				{u,v - (float(1.0f) / float(kSubdivision))},
				{(cosf(lat + kLatEvery) * cosf(lon)), (sinf(lat + kLatEvery)), (cosf(lat + kLatEvery) * sinf(lon))} });


			mesh.AddVertex({ {(cosf(lat) * cosf(lon + kLonEvery)), (sinf(lat)), (cosf(lat) * sinf(lon + kLonEvery)), 1.0f},
				{u + (float(1.0f) / float(kSubdivision)),v},
				{(cosf(lat) * cosf(lon + kLonEvery)), (sinf(lat)), (cosf(lat) * sinf(lon + kLonEvery))} });


			mesh.AddVertex({ {(cosf(lat + kLatEvery) * cosf(lon)), (sinf(lat + kLatEvery)), (cosf(lat + kLatEvery) * sinf(lon)), 1.0f},
				{u,v - (float(1.0f) / float(kSubdivision))},
				{(cosf(lat + kLatEvery) * cosf(lon)), (sinf(lat + kLatEvery)), (cosf(lat + kLatEvery) * sinf(lon))} });


			mesh.AddVertex({ {(cosf(lat + kLatEvery) * cosf(lon + kLonEvery)), (sinf(lat + kLatEvery)), (cosf(lat + kLatEvery) * sinf(lon + kLonEvery)), 1.0f},
				{u + (float(1.0f) / float(kSubdivision)),v - (float(1.0f) / float(kSubdivision))},
				{(cosf(lat + kLatEvery) * cosf(lon + kLonEvery)), (sinf(lat + kLatEvery)), (cosf(lat + kLatEvery) * sinf(lon + kLonEvery))} });


			mesh.AddVertex({ {(cosf(lat) * cosf(lon + kLonEvery)), (sinf(lat)), (cosf(lat) * sinf(lon + kLonEvery)), 1.0f},
				{u + (float(1.0f) / float(kSubdivision)),v},
				{(cosf(lat) * cosf(lon + kLonEvery)), (sinf(lat)), (cosf(lat) * sinf(lon + kLonEvery))} });
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
	aiMatrix4x4 ailocal = node->mTransformation;
	ailocal.Transpose();
	result.local.m[0][0] = ailocal[0][0];
	result.local.m[0][1] = ailocal[0][1];
	result.local.m[0][2] = ailocal[0][2];
	result.local.m[0][3] = ailocal[0][3];

	result.local.m[1][0] = ailocal[1][0];
	result.local.m[1][1] = ailocal[1][1];
	result.local.m[1][2] = ailocal[1][2];
	result.local.m[1][3] = ailocal[1][3];

	result.local.m[2][0] = ailocal[2][0];
	result.local.m[2][1] = ailocal[2][1];
	result.local.m[2][2] = ailocal[2][2];
	result.local.m[2][3] = ailocal[2][3];

	result.local.m[3][0] = ailocal[3][0];
	result.local.m[3][1] = ailocal[3][1];
	result.local.m[3][2] = ailocal[3][2];
	result.local.m[3][3] = ailocal[3][3];


	result.name = node->mName.C_Str();
	result.children.resize(node->mNumChildren);
	for (uint32_t i = 0; i < node->mNumChildren; i++) {
		result.children[i] = ReadNode(node->mChildren[i]);
	}

	return result;
}
