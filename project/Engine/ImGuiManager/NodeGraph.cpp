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
void NodeGraph::Update(ax::NodeEditor::EditorContext* ctx) {
	ClearResults();

	// リンクしているかどうか
	for (auto& node : nodes) {
		for (auto& pin : node.inputs)
			if (IsPinLinked(pin.id))
				pin.isLinked = true;
			else pin.isLinked = false;
		for (auto& pin : node.outputs)
			if (IsPinLinked(pin.id))
				pin.isLinked = true;
			else pin.isLinked = false;

		node.isUpdated = false;
	}

	// リンクしている値を取得
	for (auto& node : nodes) {
		ValueUpdate(node);
	}

	if (!ctx) {
		ctx = ax::NodeEditor::CreateEditor();
	}

	ed::SetCurrentEditor(ctx);
	ImGuiManager::GetInstance()->DrawNodeEditor(this);
	ed::SetCurrentEditor(nullptr);
}

void NodeGraph::ValueUpdate(MyNode& node) {
	std::vector<Value> inputValues;

	if (node.isUpdated) {
		return;
	} else {
		for (const Pin& input : node.inputs) {
			if (!input.isLinked) {
				// 未接続ならNone値
				inputValues.push_back(Value());
				continue;
			}

			const Link* pLink = nullptr;
			for (const Link& link : links) {
				if (link.endPinId == input.id)
					pLink = &link;
			}
			if (!pLink)
				continue;
			MyNode* srcNode = FindNodeByPinId(pLink->startPinId);
			if (!srcNode)
				continue;
			ValueUpdate(*srcNode);

			const Value* output = nullptr;
			for (int i = 0; i < srcNode->outputs.size(); i++) {
				if (pLink->startPinId == srcNode->outputs[i].id) {
					output = &srcNode->outputValue[i];
				}
			}
			if (output)
				inputValues.push_back(*output);
			else
				inputValues.push_back(Value()); // 安全のためNone
		}

		node.outputValue.clear();
		if (!inputValues.empty()) {
			if (node.type == MyNode::NodeType::Material) {
				node.outputValue.push_back(inputValues[0].type != Value::Type::Texture ? node.values[0] : inputValues[0]);
				node.outputValue.push_back(inputValues[1].type != Value::Type::Color ? node.values[1] : inputValues[1]);
			}

			if (node.type == MyNode::NodeType::Texture) {
				node.outputValue.push_back(inputValues[0].type != Value::Type::Texture ? node.values[0] : inputValues[0]);
			}
			// 随時追加


		} else {
			node.outputValue = node.values;
		}
		node.isUpdated = true;
	}
}

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

MyNode* NodeGraph::FindNodeByPinId(ed::PinId pinId) {
	for (MyNode& node : nodes) {
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
		inputs.push_back({ ImGuiManager::GetInstance()->GeneratePinId(), false, Pin::Type::Input, PinType::Texture });
		outputs.push_back({ ImGuiManager::GetInstance()->GeneratePinId(), false, Pin::Type::Output, PinType::Texture });

		break;
	case MyNode::NodeType::Float:
		break;
	case MyNode::NodeType::Add:
		break;
	case MyNode::NodeType::Material:

		name = "Material";
		type = MyNode::NodeType::Material;
		inputs.push_back({ ImGuiManager::GetInstance()->GeneratePinId(), false, Pin::Type::Input, PinType::Texture });
		inputs.push_back({ ImGuiManager::GetInstance()->GeneratePinId(), false, Pin::Type::Input, PinType::Color });
		values.push_back(Value("white2x2.png"));
		values.push_back(Value(Vector4(1.0f, 1.0f, 1.0f, 1.0f)));
		evaluator = [](const std::vector<Value>& inputs) {
			return !inputs.empty() ? inputs[0] : Value();
			};
		break;
	case MyNode::NodeType::Color:

		name = "Color";
		type = MyNode::NodeType::Color;
		outputs.push_back({ ImGuiManager::GetInstance()->GeneratePinId(), false, Pin::Type::Output, PinType::Color });
		values.push_back(Value(Vector4(1.0f, 1.0f, 1.0f, 1.0f)));
		evaluator = [](const std::vector<Value>& inputs) {
			return !inputs.empty() ? inputs[0] : Value();
			};

		break;
	default:
		break;
	}
}
#endif // _DEBUG
