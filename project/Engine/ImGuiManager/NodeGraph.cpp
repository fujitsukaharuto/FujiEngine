#include "NodeGraph.h"
#ifdef _DEBUG
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#include "ImGuizmo.h"
#include "imgui_node_editor.h"

namespace ed = ax::NodeEditor;
#endif // _DEBUG


#ifdef _DEBUG
MyNode& NodeGraph::AddNode(const MyNode& node) {
	nodes.push_back(node);
	return nodes.back();
}

void NodeGraph::AddLink(const Link& link) {
	links.push_back(link);
}

Value NodeGraph::EvaluateNode(const MyNode& node) {
	// すでに値が入っていればそれを返す（キャッシュ的な使い方）
	if (node.result.type != Value::Type::None) {
		return node.result;
	}

	std::vector<Value> inputValues;

	for (const Pin& input : node.inputs) {
		// 入力ピンに繋がっているリンクを探す
		for (const Link& link : links) {
			if (link.endPinId == input.id) {
				const MyNode* srcNode = FindNodeByPinId(link.startPinId);
				if (srcNode) {
					Value v = EvaluateNode(*srcNode);
					inputValues.push_back(v);
				}
			}
		}
	}

	// 入力を使って評価関数を呼ぶ
	if (node.evaluator) {
		const_cast<MyNode&>(node).result = node.evaluator(inputValues);
	}

	return node.result;
}

const MyNode* NodeGraph::FindNodeByPinId(ed::PinId pinId) const {
	for (const auto& node : nodes) {
		for (const auto& pin : node.outputs) {
			if (pin.id == pinId) return &node;
		}
		for (const auto& pin : node.inputs) {
			if (pin.id == pinId) return &node;
		}
	}
	return nullptr;
}

MyNode* NodeGraph::FindNodeById(ed::NodeId id) {
	for (auto& node : nodes) {
		if (node.id == id) return &node;
	}
	return nullptr;
}

void NodeGraph::ClearResults() {
	for (auto& node : nodes) {
		node.result = Value(); // type = None にリセット
	}
}



#endif // _DEBUG
