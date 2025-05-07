#include "Object3d.h"
#include "ModelManager.h"
#include "DXCom.h"
#include "LightManager.h"
#include "CameraManager.h"
#include "Engine/ImGuiManager/ImGuiManager.h"

Object3d::Object3d() {
	dxcommon_ = ModelManager::GetInstance()->ShareDXCom();
	lightManager_ = ModelManager::GetInstance()->ShareLight();
}

Object3d::~Object3d() {
	dxcommon_ = nullptr;
}

void Object3d::Create(const std::string& fileName) {
	this->camera_ = CameraManager::GetInstance()->GetCamera();
	ModelManager::GetInstance()->LoadOBJ(fileName);
	SetModel(fileName);
	transform = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	nowTextureName = model_->GetTextuerName();
	CreateWVP();
}

void Object3d::CreateSphere() {
	this->camera_ = CameraManager::GetInstance()->GetCamera();
	ModelManager::GetInstance()->CreateSphere();
	SetModel("Sphere");
	transform = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	CreateWVP();
}

void Object3d::Draw(Material* mate) {
	SetWVP();

	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	cList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
	cList->SetGraphicsRootConstantBufferView(4, cameraPosResource_->GetGPUVirtualAddress());

	if (model_) {
		model_->Draw(cList, mate);
	}
}

void Object3d::AnimeDraw() {
	SetBillboardWVP();

	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	cList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
	cList->SetGraphicsRootConstantBufferView(4, cameraPosResource_->GetGPUVirtualAddress());
	lightManager_->SetLightCommand(cList);

	if (model_) {
		model_->Draw(cList, nullptr);
	}
}

Matrix4x4 Object3d::GetWorldMat() const {
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

	if (parent_) {
		const Matrix4x4& parentWorldMatrix = parent_->GetWorldMat();
		worldMatrix = Multiply(worldMatrix, parentWorldMatrix);
	} else if (isCameraParent_) {
		const Matrix4x4& parentWorldMatrix = camera_->GetWorldMatrix();
		worldMatrix = Multiply(worldMatrix, parentWorldMatrix);
	}

	return worldMatrix;
}

Vector3 Object3d::GetWorldPos() const {

	Matrix4x4 worldM = GetWorldMat();
	Vector3 worldPos = { worldM.m[3][0],worldM.m[3][1] ,worldM.m[3][2] };

	return worldPos;
}



void Object3d::DebugGUI() {
#ifdef _DEBUG
	ImGui::Indent();
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Selected;
	if (ImGui::TreeNodeEx("Trans",flags)) {
	ImGui::DragFloat3("position", &transform.translate.x, 0.01f);
	ImGui::DragFloat3("rotate", &transform.rotate.x, 0.01f);
	ImGui::DragFloat3("scale", &transform.scale.x, 0.01f);
		ImGui::TreePop();
	}
	if (ImGui::TreeNodeEx("color",flags)) {
		Vector4 color = model_->GetColor(0);
		ImGui::ColorEdit4("color", &color.x);
		SetColor(color);
		ImGui::TreePop();
	}
	ImGui::Unindent();
#endif // _DEBUG
}

void Object3d::SetColor(const Vector4& color) {
	model_->SetColor(color);
}

void Object3d::SetUVScale(const Vector2& scale, const Vector2& uvTrans) {
	model_->SetUVScale(scale, uvTrans);
}

void Object3d::SetTexture(const std::string& name) {
	if (name == nowTextureName) {
		return;
	}
	model_->SetTexture(name);
	nowTextureName = name;
}

void Object3d::SetLightEnable(LightMode mode) {
	model_->SetLightEnable(mode);
}

void Object3d::SetModel(const std::string& fileName) {
	model_ = std::make_unique<Model>(*(ModelManager::FindModel(fileName)));
}

void Object3d::CreateWVP() {
	wvpResource_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(TransformationMatrix));
	wvpDate_ = nullptr;
	wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpDate_));
	wvpDate_->WVP = MakeIdentity4x4();
	wvpDate_->World = MakeIdentity4x4();
	wvpDate_->WorldInverseTransPose = Transpose(Inverse(wvpDate_->World));

	cameraPosResource_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(DirectionalLight));
	cameraPosData_ = nullptr;
	cameraPosResource_->Map(0, nullptr, reinterpret_cast<void**>(&cameraPosData_));
	cameraPosData_->worldPosition = camera_->transform.translate;
}

void Object3d::SetWVP() {
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	Matrix4x4 worldViewProjectionMatrix;


	if (parent_) {
		const Matrix4x4& parentWorldMatrix = parent_->GetWorldMat();
		worldMatrix = Multiply(worldMatrix, parentWorldMatrix);
	} else if (isCameraParent_) {
		const Matrix4x4& parentWorldMatrix = camera_->GetWorldMatrix();
		worldMatrix = Multiply(worldMatrix, parentWorldMatrix);
	}


	if (camera_) {
		const Matrix4x4& viewProjectionMatrix = camera_->GetViewProjectionMatrix();
		worldViewProjectionMatrix = Multiply(worldMatrix, viewProjectionMatrix);
	} else {
		worldViewProjectionMatrix = worldMatrix;
	}

	wvpDate_->World = Multiply(model_->data_.rootNode.local, worldMatrix);
	wvpDate_->WVP = Multiply(model_->data_.rootNode.local, worldViewProjectionMatrix);
	wvpDate_->WorldInverseTransPose = Transpose(Inverse(wvpDate_->World));

	cameraPosData_->worldPosition = camera_->transform.translate;

}

void Object3d::SetBillboardWVP() {
	Matrix4x4 worldViewProjectionMatrix;
	Matrix4x4 worldMatrix = MakeIdentity4x4();


	worldMatrix = Multiply(MakeScaleMatrix(transform.scale), MakeRotateXYZMatrix(transform.rotate));
	worldMatrix = Multiply(worldMatrix, billboardMatrix_);
	worldMatrix = Multiply(worldMatrix, MakeTranslateMatrix(transform.translate));

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