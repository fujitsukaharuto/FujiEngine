#pragma once
#include <vector>
#include <string>
#include <functional>
#include <cstdint>
#include <variant>
#include "Engine/Math/Vector/Vector2.h"
#include "Engine/Math/Vector/Vector3.h"
#include "Engine/Math/Vector/Vector4.h"
#ifdef _DEBUG
#include "imgui.h"
#include "imgui_node_editor.h"

namespace ed = ax::NodeEditor;
#endif // _DEBUG

#ifdef _DEBUG
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
	Color,
	Texture,
};

enum class PinKind {
	Output,
	Input
};

struct Pin {
	ed::PinId id;
	bool isLinked = false;
	enum class Type { Input, Output } type;
	PinType pinType;
};

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
		Color,
		// 追加予定のノード種類…
	} type;

	bool isUpdated = false;

	Value result; // ← ★ これがノードの出力

	std::string texName;

	std::function<Value(const std::vector<Value>&)> evaluator; // 入力 → 出力

	void CreateNode(NodeType nodeType);
};

struct Link {
	ed::LinkId id;
	ed::PinId startPinId; // output
	ed::PinId endPinId;   // input
};
#endif // _DEBUG



// NodeEditor
#ifdef _DEBUG
class NodeGraph {
public:
	// ノードとリンクのリスト
	std::vector<MyNode> nodes;
	std::vector<Link> links;

	void Update(ax::NodeEditor::EditorContext* ctx);
	void ValueUpdate(MyNode& node);

	// ノードの追加
	MyNode& AddNode(const MyNode& node);

	// リンクの追加
	void AddLink(const Link& link);

	// 指定ノードを評価して出力を得る（再帰的）
	Value EvaluateNode(const MyNode& node);

	// ピンIDからノードを探す
	MyNode* FindNodeByPinId(ed::PinId pinId);

	// ノードIDからノードを探す
	MyNode* FindNodeById(ed::NodeId id);

	bool IsPinLinked(ed::PinId pinId) const;

	// キャッシュ（result）をクリアする
	void ClearResults();
};
#endif // _DEBUG