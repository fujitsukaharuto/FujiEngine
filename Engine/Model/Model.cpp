#include "Model.h"
#include "DXCom.h"

#include <fstream>
#include <sstream>


Model::Model() {}

Model::~Model() {}

Model* Model::CreateOBJ(const std::string& filename)
{
	Model* model = new Model();

	Mesh mesh{};
	Material material{};


	std::vector<Vector4> positions;
	std::vector<Vector3> normals;
	std::vector<Vector2> texcords;
	std::string line;

	std::string path = kDirectoryPath_ + "/" + filename;

	std::ifstream file(path);
	assert(file.is_open());

	while (std::getline(file, line))
	{
		std::string identifier;
		std::istringstream s(line);

		s >> identifier;

		if (identifier == "v")
		{
			Vector4 position;
			s >> position.X >> position.Y >> position.Z;
			position.W = 1.0f;

			positions.push_back(position);
		}
		else if (identifier == "vt")
		{
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			texcords.push_back(texcoord);
		}
		else if (identifier == "vn")
		{
			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			normals.push_back(normal);
		}
		else if (identifier == "f")
		{
			VertexDate triangle[3];
			for (int32_t faceVertex = 0; faceVertex < 3; faceVertex++)
			{
				std::string vertexDefinition;
				s >> vertexDefinition;
				//頂点の要素へのindexは　位置/uv/法線　で格納されているので、分解してindex取得
				std::istringstream v(vertexDefinition);
				std::string index;
				uint32_t elementIndices[3] = { 0,0,0 };
				int32_t element = 0;
				while (std::getline(v, index, '/'))
				{
					if (!index.empty())
					{
						elementIndices[element] = std::stoi(index);
					}
					element++;
				}
				Vector4 position = positions[elementIndices[0] - 1];
				position.X *= -1.0f;
				Vector2 texcoord = { 0.0f,0.0f };
				if (elementIndices[1] > 0)
				{
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
		else if (identifier == "mtllib")
		{
			std::string materialFilename;
			s >> materialFilename;

			MaterialDataPath matelialPath;
			matelialPath = LoadMaterialFile(materialFilename);

			material.SetTextureNamePath(matelialPath.textureFilePath);
			material.CreateMaterial();
			model->material_.push_back(material);
		}
	}

	mesh.CreateMesh();

	model->wvpResource_ = DXCom::GetInstance()->CreateBufferResource(DXCom::GetInstance()->GetDevice(), sizeof(TransformationMatrix));
	model->wvpDate_ = nullptr;
	model->wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&model->wvpDate_));
	model->wvpDate_->WVP = MakeIdentity4x4();
	model->wvpDate_->World = MakeIdentity4x4();

	model->mesh_.push_back(mesh);

	return model;
}

Model* Model::CreateSphere()
{

	Model* model = new Model();

	Mesh mesh{};
	Material material{};


	const float pi = 3.1415926535f;
	const uint32_t kSubdivision = 16;
	const float kLonEvery = (pi * 2.0f) / static_cast<float>(kSubdivision);
	const float kLatEvery = (pi) / static_cast<float>(kSubdivision);

	for (uint32_t latIndex = 0; latIndex < kSubdivision; latIndex++)
	{
		float lat = -pi / 2.0f + kLatEvery * latIndex;
		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; lonIndex++)
		{
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

	
	model->wvpResource_ = DXCom::GetInstance()->CreateBufferResource(DXCom::GetInstance()->GetDevice(), sizeof(TransformationMatrix));
	model->wvpDate_ = nullptr;
	model->wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&model->wvpDate_));
	model->wvpDate_->WVP = MakeIdentity4x4();
	model->wvpDate_->World = MakeIdentity4x4();

	model->mesh_.push_back(mesh);
	model->material_.push_back(material);

	return model;
}

void Model::Draw()
{

	ID3D12GraphicsCommandList* cList = DXCom::GetInstance()->GetCommandList();

	for (uint32_t index = 0; index < mesh_.size(); ++index) {
		cList->SetGraphicsRootConstantBufferView(0, material_[index].GetMaterialResource()->GetGPUVirtualAddress());
		cList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
		cList->SetGraphicsRootConstantBufferView(3, material_[index].GetDirectionLight()->GetGPUVirtualAddress());
		cList->SetGraphicsRootDescriptorTable(2, material_[index].GetTexture()->gpuHandle);
		mesh_[index].Draw(cList);
	}

}

void Model::SetWVP()
{
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	Matrix4x4 viewMatrix = DXCom::GetInstance()->GetView();

	Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, DXCom::GetInstance()->GetAspect(), 0.1f, 100.0f);
	Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));

	wvpDate_->World = worldMatrix;
	wvpDate_->WVP = worldViewProjectionMatrix;
}

MaterialDataPath Model::LoadMaterialFile(const std::string& filename)
{
	MaterialDataPath materialData;
	std::string line;
	std::ifstream file(kDirectoryPath_ + filename);
	assert(file.is_open());

	while (std::getline(file, line))
	{
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		if (identifier == "map_Kd")
		{
			std::string textureFilename;
			s >> textureFilename;
			materialData.textureFilePath = textureFilename;
		}
	}

	if (materialData.textureFilePath.empty())
	{
		std::string whiteTexture = "white2x2.png";
		materialData.textureFilePath = whiteTexture;
	}

	return materialData;
}
