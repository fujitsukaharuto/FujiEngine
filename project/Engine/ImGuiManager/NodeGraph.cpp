#include "NodeGraph.h"
#include "Engine/ImGuiManager/ImGuiManager.h"
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

bool NodeGraph::IsPinLinked(ed::PinId pinId) const {
	for (const auto& link : links) {
		if (link.startPinId == pinId || link.endPinId == pinId) {
			return true;
		}
	}
	return false;
}

void NodeGraph::ClearResults() {

}



#endif // _DEBUG

#ifdef _DEBUG
void MyNode::CreateNode(NodeType nodeType) {

	id = ImGuiManager::GetInstance()->GenerateNodeId();
	switch (nodeType) {
	case MyNode::NodeType::Texture:

		name = "TextureInput";
		type = MyNode::NodeType::Texture;
		inputs.push_back({ ImGuiManager::GetInstance()->GeneratePinId(), false, Pin::Type::Input });
		outputs.push_back({ ImGuiManager::GetInstance()->GeneratePinId(), false, Pin::Type::Output });

		break;
	case MyNode::NodeType::Float:
		break;
	case MyNode::NodeType::Add:
		break;
	case MyNode::NodeType::Material:

		name = "Material";
		type = MyNode::NodeType::Material;
		inputs.push_back({ ImGuiManager::GetInstance()->GeneratePinId(), false, Pin::Type::Input });
		evaluator = [](const std::vector<Value>& inputs) {
			return !inputs.empty() ? inputs[0] : Value();
			};

		break;
	default:
		break;
	}
}
#endif // _DEBUG
