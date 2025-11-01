#pragma once
#include <vector>
#include <string>
#include <functional>
#include <cstdint>
#include <variant>
#include "Engine/Math/Vector/Vector2.h"
#include "Engine/Math/Vector/Vector3.h"
#include "Engine/Math/Vector/Vector4.h"
#include "Engine/Editor/JsonSerializer.h"
#ifdef _DEBUG
#include "imgui.h"
#include "imgui_node_editor.h"

namespace ed = ax::NodeEditor;
#endif // _DEBUG

#ifdef _DEBUG
/// <summary>
/// ノードが持つ値
/// </summary>
struct Value {

	enum class Type { None, Int, Float, Vector2, Vector3, Color, Texture };

	// 実データ本体（variant にすべて詰め込む）
	std::variant<std::monostate, int, float, Vector2, Vector3, Vector4, std::string> data;

	// タイプを明示的に持っておく（オプション）
	Type type = Type::None;

	// コンストラクタ（型推論対応）
	Value() : data(std::monostate{}), type(Type::None) {}
	Value(int v) : data(v), type(Type::Int) {}
	Value(float v) : data(v), type(Type::Float) {}
	Value(const Vector2& v) : data(v), type(Type::Vector2) {}
	Value(const Vector3& v) : data(v), type(Type::Vector3) {}
	Value(const Vector4& v) : data(v), type(Type::Color) {}
	Value(const std::string& texName) : data(texName), type(Type::Texture) {}
	Value(const char* texName) : data(std::string(texName)), type(Type::Texture) {}

	// ヘルパー関数
	bool IsValid() const {
		return type != Type::None;
	}

	// 型チェック（テンプレートベース）
	template<typename T>
	bool Is() const {
		return std::holds_alternative<T>(data);
	}

	// 値取得（安全な参照取得、存在しなければ例外）
	template<typename T>
	const T& Get() const {
		return std::get<T>(data);
	}
	template<typename T>
	T& Get() {
		return std::get<T>(data);
	}

	// 値取得（デフォルト値付きで安全）
	template<typename T>
	T GetOr(const T& defaultValue) const {
		if (std::holds_alternative<T>(data)) {
			return std::get<T>(data);
		}
		return defaultValue;
	}
};

enum class PinType {
	Flow,
	Bool,
	Int,
	Float,
	String,
	Object,
	Function,
	Delegate,
	Vector2,
	Color,
	Texture,
	Mateial,
};

enum class PinKind {
	Output,
	Input
};

enum class AddType {
	Increment,
	DeltaTime,
};

/// <summary>
/// NodeのPin
/// </summary>
struct Pin {
	ed::PinId id;
	bool isLinked = false;
	enum class Type { Input, Output } type;
	PinType pinType;
};

/// <summary>
/// Nodeのデータ
/// </summary>
struct MyNode {
	ed::NodeId id;
	std::string name;
	std::vector<Pin> inputs;
	std::vector<Pin> outputs;
	std::vector<Value> values;
	std::vector<Value> outputValue;

	enum class NodeType {
		Texture,
		Float,
		Add,
		Material,
		SubMaterial,
		Color,
		Vector2,
		// 追加予定のノード種類…
	} type;

	bool isUpdated = false;

	Value result; // ← ★ これがノードの出力

	// NodeTypeによって必要になるもの
	std::string texName;
	AddType addType = AddType::Increment;

	MyNode* child = nullptr;
	std::function<Value(const std::vector<Value>&)> evaluator; // 入力 → 出力

	void CreateNode(NodeType nodeType);
};

/// <summary>
/// NodeのLinkデータ
/// </summary>
struct Link {
	ed::LinkId id;
	ed::PinId startPinId; // output
	ed::PinId endPinId;   // input
};
#endif // _DEBUG



// NodeEditor
#ifdef _DEBUG
/// <summary>
/// NodeGraphのクラス
/// </summary>
class NodeGraph {
public:
	// ノードとリンクのリスト
	std::vector<MyNode> nodes;
	std::vector<Link> links;

	void Update(ax::NodeEditor::EditorContext* ctx);
	void ValueUpdate(MyNode& node);
	void NameUpdate(MyNode& parentNode, MyNode& node,int inputNum);

	/// <summary>Nodeの追加</summary>
	MyNode& AddNode(const MyNode& node);

	/// <summary>リンクの追加</summary>
	void AddLink(const Link& link);

	/// <summary>指定ノードを評価して出力を得る（再帰的）</summary>
	Value EvaluateNode(const MyNode& node);

	/// <summary>ピンIDからノードを探す</summary>
	MyNode* FindNodeByPinId(ed::PinId pinId);

	/// <summary>ノードIDからノードを探す</summary>
	MyNode* FindNodeById(ed::NodeId id);

	/// <summary>ピンが既に繋がっているか</summary>
	bool IsPinLinked(ed::PinId pinId) const;

	/// <summary>キャッシュをクリアする</summary>
	void ClearResults();


	/// <summary>NodeTypeを文字列に</summary>
	std::string NodeTypeToString(MyNode::NodeType t);
	/// <summary>文字列をNodeTypeに</summary>
	MyNode::NodeType StringToNodeType(const std::string& str);

	/// <summary>NodeDetaを出力</summary>
	json SaveNodeData();
	/// <summary>Valueを出力</summary>
	json SerializeValue(const Value& v);
	/// <summary>Nodeを出力</summary>
	json SerializeNode(const MyNode& node);

	/// <summary>NodeDetaを読み込み</summary>
	ed::NodeId DeserializeNodeData(const std::string& filePath);
	/// <summary>Nodeを読み込み</summary>
	MyNode DeserializeNode(const json& j);
	/// <summary>Valueを出力</summary>
	Value DeserializeValue(const json& j);

private:
	ed::NodeId materialNodeId_;
};
#endif // _DEBUG


struct NodeContent {

	bool isMoveUV_ = false;
	bool isAddDeltaUV_ = false;

	Vector2 incrementUV_;

};