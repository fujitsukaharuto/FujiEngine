#pragma once
#include <vector>
#include <string>
#include <functional>
#include <cstdint>
#ifdef _DEBUG
#include "imgui.h"
#include "imgui_node_editor.h"

namespace ed = ax::NodeEditor;
#endif // _DEBUG

#ifdef _DEBUG
struct Value {
	enum class Type { None, Int, Float, Vector2, Vector3, Texture } type = Type::None;

	union {
		int     intValue;
		float   floatValue;
		float   vec2Value[2];
		float   vec3Value[3];
	};

	std::string textureName;

	Value() : type(Type::None), intValue(0) {}
};

struct Pin {
	ed::PinId id;
	bool isLinked = false;
	enum class Type { Input, Output } type;
};

struct MyNode {
	ed::NodeId id;
	std::string name;
	std::vector<Pin> inputs;
	std::vector<Pin> outputs;

	enum class NodeType {
		Texture,
		Float,
		Add,
		Selector,
		// 追加予定のノード種類…
	} type;

	Value result; // ← ★ これがノードの出力

	std::function<Value(const std::vector<Value>&)> evaluator; // 入力 → 出力
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

	// ノードの追加
	MyNode& AddNode(const MyNode& node);

	// リンクの追加
	void AddLink(const Link& link);

	// 指定ノードを評価して出力を得る（再帰的）
	Value EvaluateNode(const MyNode& node);

	// ピンIDからノードを探す
	const MyNode* FindNodeByPinId(ed::PinId pinId) const;

	// ノードIDからノードを探す
	MyNode* FindNodeById(ed::NodeId id);

	bool IsPinLinked(ed::PinId pinId) const;

	// キャッシュ（result）をクリアする
	void ClearResults();
};
#endif // _DEBUG