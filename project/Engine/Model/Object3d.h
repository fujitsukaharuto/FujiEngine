#pragma once
#include <wrl/client.h>
#include <string>
#include "Model.h"
#include "Camera.h"
#include "Object3dCommon.h"
#ifdef _DEBUG
#include "imgui_node_editor.h"
#include "Engine/ImGuiManager/NodeGraph.h"
#endif


class DXCom;
class LightManager;
class PointLight;
class SpotLight;

class Object3d {
public:
	Object3d();
	~Object3d();

public:

	void Create(const std::string& fileName);

	void CreateSphere();

	void CreateRing(float out = 1.0f, float in = 0.2f, float radius = 2.0f);

	void CreateSylinder(float topRadius = 1.0f, float bottomRadius = 1.0f, float height = 3.0f);

	void Draw(Material* mate = nullptr);

	void AnimeDraw();

	Matrix4x4 GetWorldMat() const;

	Vector3 GetWorldPos()const;

	void UpdateWVP() { SetWVP(); }

	void DebugGUI();

	void LoadTransformFromJson(const std::string& filename);

	void SetColor(const Vector4& color);

	void SetUVScale(const Vector2& scale, const Vector2& uvTrans);

	void SetCamera(Camera* camera) { this->camera_ = camera; }

	void SetParent(Object3d* parent) { parent_ = parent; }

	void SetCameraParent(bool is) { isCameraParent_ = is; }

	void SetTexture(const std::string& name);

	void SetBillboardMat(const Matrix4x4& mat) { billboardMatrix_ = mat; }

	void SetLightEnable(LightMode mode);

	void SetModel(const std::string& fileName);

	Trans transform{};

private:

	void CreateWVP();

	void SetWVP();

	void SetBillboardWVP();

	void CreatePropertyCommand(int type);

	void SetTextureNode();

private:
	Object3dCommon* common_;
	std::unique_ptr<Model> model_ = nullptr;

	DXCom* dxcommon_;
	LightManager* lightManager_;
	Camera* camera_;
	Object3d* parent_ = nullptr;

	bool isCameraParent_ = false;

	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_ = nullptr;
	TransformationMatrix* wvpDate_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> cameraPosResource_ = nullptr;
	CameraForGPU* cameraPosData_ = nullptr;

	Matrix4x4 billboardMatrix_;
	std::string nowTextureName;

	Vector3 prevPos_;
	Vector3 prevRotate_;
	Vector3 prevScale_;
	int guizmoType_ = 0;
	float IsUsingGuizmo_ = false;
#ifdef _DEBUG
	ax::NodeEditor::EditorContext* nodeEditorContext_ = nullptr;
	NodeGraph nodeGraph_;
	ed::NodeId selectorNodeId_;
#endif // _DEBUG
};