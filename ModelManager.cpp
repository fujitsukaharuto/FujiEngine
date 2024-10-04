#include "ModelManager.h"
#include <fstream>
#include <sstream>



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


Model* ModelManager::LoadOBJ(const std::string& filename) {
	ModelManager* instance = GetInstance();

	auto iterator = instance->models_.find(filename);
	if (iterator != instance->models_.end()) {
		return new Model(*(*iterator).second.get());
	}


	Model* model = new Model();
	Mesh mesh{};
	Material material{};


	std::vector<Vector4> positions;
	std::vector<Vector3> normals;
	std::vector<Vector2> texcords;
	std::string line;

	std::string path = instance->kDirectoryPath_ + "/" + filename;
	std::ifstream file(path);
	assert(file.is_open());

	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);

		s >> identifier;

		if (identifier == "v") {
			Vector4 position;
			s >> position.X >> position.Y >> position.Z;
			position.W = 1.0f;

			positions.push_back(position);
		}
		else if (identifier == "vt") {
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;

			texcords.push_back(texcoord);
		}
		else if (identifier == "vn") {
			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;

			normals.push_back(normal);
		}
		else if (identifier == "f") {
			VertexDate triangle[3];
			for (int32_t faceVertex = 0; faceVertex < 3; faceVertex++) {
				std::string vertexDefinition;
				s >> vertexDefinition;
				//頂点の要素へのindexは　位置/uv/法線　で格納されているので、分解してindex取得
				std::istringstream v(vertexDefinition);
				std::string index;
				uint32_t elementIndices[3] = { 0,0,0 };
				int32_t element = 0;
				while (std::getline(v, index, '/')) {
					if (!index.empty()) {
						elementIndices[element] = std::stoi(index);
					}
					element++;
				}
				Vector4 position = positions[elementIndices[0] - 1];
				position.X *= -1.0f;
				Vector2 texcoord = { 0.0f,0.0f };
				if (elementIndices[1] > 0) {
					texcoord = texcords[elementIndices[1] - 1];
					texcoord.y = 1.0f - texcoord.y;
				}
				Vector3 normal = normals[elementIndices[2] - 1];
				normal.x *= -1.0f;
				triangle[faceVertex] = { position,texcoord,normal };
				/*VertexData vertex= { position,texcoord,normal };
				modeldata.vertices.push_back(vertex);*/
			}
			mesh.AddVertex({ {triangle[2].position},{triangle[2].texcoord},{triangle[2].normal} });
			mesh.AddVertex({ {triangle[1].position},{triangle[1].texcoord},{triangle[1].normal} });
			mesh.AddVertex({ {triangle[0].position},{triangle[0].texcoord},{triangle[0].normal} });
		}
		else if (identifier == "mtllib") {
			std::string materialFilename;
			s >> materialFilename;

			MaterialDataPath matelialPath;
			matelialPath = LoadMaterialFile(materialFilename);

			material.SetTextureNamePath(matelialPath.textureFilePath);
			material.CreateMaterial();
			model->AddMaterial(material);
		}
	}

	mesh.CreateMesh();

	model->AddMesh(mesh);

	instance->AddModel(filename, model);

	return model;
}


Model* ModelManager::FindModel(const std::string& filename) {
	ModelManager* instance = GetInstance();
	auto iterator = instance->models_.find(filename);
	if (iterator != instance->models_.end()) {
		return (*iterator).second.get();
	}
	return nullptr;
}


Model* ModelManager::CreateSphere() {

	ModelManager* instance = GetInstance();

	auto iterator = instance->models_.find("Sphere");
	if (iterator != instance->models_.end()) {
		return new Model(*(*iterator).second.get());
	}


	Model* model = new Model();
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

	instance->AddModel("Sphere", model);
	return model;
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
