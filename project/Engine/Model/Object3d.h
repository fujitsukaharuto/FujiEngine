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

struct ObjIDData {
	int objID;
	uint32_t padding[3];
};

class Object3d {
public:
	Object3d();
	~Object3d();

public:

	void Create(const std::string& fileName);

	void CreateSphere();

	/// <summary>
	/// Ringの作成
	/// </summary>
	/// <param name="out">外側の半径(デフォルトは1.0f)</param>
	/// <param name="in">内側の半径(デフォルトは0.2f)</param>
	/// <param name="radius">円周(デフォルトは2.0f)</param>
	/// <param name="horizon">水平にするか(デフォルトはfalse)</param>
	void CreateRing(float out = 1.0f, float in = 0.2f, float radius = 2.0f, bool horizon = false);

	/// <summary>
	/// Cylinderの作成
	/// </summary>
	/// <param name="topRadius">上の半径(デフォルトは1.0f)</param>
	/// <param name="bottomRadius">下の半径(デフォルトは1.0f)</param>
	/// <param name="height">高さ(デフォルトは3.0f)</param>
	void CreateCylinder(float topRadius = 1.0f, float bottomRadius = 1.0f, float height = 3.0f);

	void Draw(Material* mate = nullptr, bool isAdd = false);

	/// <summary>
	/// 連番用描画
	/// </summary>
	void AnimeDraw();

	Matrix4x4 GetWorldMat() const;

	Vector3 GetWorldPos()const;

	std::string GetModelName() { return modelName_; }

	bool IsHaveParent() { return transform.parent ? true : false; }

	void UpdateWVP() { SetWVP(); }

	void DebugGUI();

	/// <summary>
	/// jsonからTransform初期化
	/// </summary>
	void LoadTransformFromJson(const std::string& filename);

	void SetColor(const Vector4& color);

	void SetUVScale(const Vector2& scale, const Vector2& uvTrans);

	/// <summary>
	/// α値の閾値
	/// </summary>
	void SetAlphaRef(float ref);

	void SetCamera(Camera* camera) { this->camera_ = camera; }

	void SetParent(Trans* parent) { transform.parent = parent; }
	void SetAnimParent(Matrix4x4* parent) { transform.animParent = parent; }

	/// <summary>
	/// 非スケーリング継承
	/// </summary>
	void SetNoneScaleParent(bool is) { transform.isNoneScaleParent = is; }

	void SetCameraParent(bool is) { transform.isCameraParent = is; }

	void SetTexture(const std::string& name);

	void SetBillboardMat(const Matrix4x4& mat) { billboardMatrix_ = mat; }

	void SetLightEnable(LightMode mode);

	void SetModel(const std::string& fileName, bool overWrite = false);

	Trans transform{};

	int GetObjID() { return objIDData_->objID; }

	void SetEditorObjParameter();

	// MeshDraw
	void MeshDraw(Material* mate = nullptr, int drawCount = 1);

private:

	void CreateWVP();

	void SetWVP();

	void SetBillboardWVP();

	void CreatePropertyCommand(int type);

	void SetTextureNode();

private:
	std::unique_ptr<Model> model_ = nullptr;
	std::string modelName_;

	DXCom* dxcommon_;
	LightManager* lightManager_;
	Camera* camera_;

	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_ = nullptr;
	TransformationMatrix* wvpDate_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> cameraPosResource_ = nullptr;
	CameraForGPU* cameraPosData_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> objIDDataResource_ = nullptr;
	ObjIDData* objIDData_ = nullptr;
	static int useObjID_;

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