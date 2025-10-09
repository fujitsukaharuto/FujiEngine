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
				node.outputValue.push_back(inputValues[2].type != Value::Type::Vector2 ? node.values[2] : inputValues[2]);
			}

			if (node.type == MyNode::NodeType::Texture) {
				node.outputValue.push_back(inputValues[0].type != Value::Type::Texture ? node.values[0] : inputValues[0]);
			}

			if (node.type == MyNode::NodeType::Vector2) {
				Value out = node.values[0];
				if (inputValues[0].type == Value::Type::Float) out.Get<Vector2>().x = inputValues[0].Get<float>();
				if (inputValues[1].type == Value::Type::Float) out.Get<Vector2>().y = inputValues[1].Get<float>();
				node.outputValue.push_back(out);
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

std::string NodeGraph::NodeTypeToString(MyNode::NodeType t) {
	switch (t) {
	case MyNode::NodeType::Texture: return "Texture";
	case MyNode::NodeType::Float: return "Float";
	case MyNode::NodeType::Add: return "Add";
	case MyNode::NodeType::Material: return "Material";
	case MyNode::NodeType::Color: return "Color";
	case MyNode::NodeType::Vector2: return "Vector2";
	default: return "Unknown";
	}
}

MyNode::NodeType NodeGraph::StringToNodeType(const std::string& str) {
	if (str == "Texture") return MyNode::NodeType::Texture;
	if (str == "Float") return MyNode::NodeType::Float;
	if (str == "Add") return MyNode::NodeType::Add;
	if (str == "Material") return MyNode::NodeType::Material;
	if (str == "Color") return MyNode::NodeType::Color;
	if (str == "Vector2") return MyNode::NodeType::Vector2;
	return MyNode::NodeType::Vector2;
}

json NodeGraph::SaveNodeData() {
	json root;
	root["Nodes"] = json::array();
	for (const auto& n : nodes) {
		if (n.outputs.size() != 0) {
			if (n.outputs[0].isLinked == true) {
				continue;
			}
		}
		// outputがどこにも繋がってない、もしくはないノードのみを書く
		root["Nodes"].push_back(SerializeNode(n));
	}
	return root;
}

json NodeGraph::SerializeValue(const Value& v) {
	json j;
	switch (v.type) {
	case Value::Type::None:
		j["type"] = "None";
		j["value"] = nullptr;
		break;
	case Value::Type::Int:
		j["type"] = "Int";
		j["value"] = std::get<int>(v.data);
		break;
	case Value::Type::Float:
		j["type"] = "Float";
		j["value"] = std::get<float>(v.data);
		break;
	case Value::Type::Vector2: {
		const auto& vv = std::get<Vector2>(v.data);
		j["type"] = "Vector2";
		j["value"] = { vv.x, vv.y };
		break;
	}
	case Value::Type::Vector3: {
		const auto& vv = std::get<Vector3>(v.data);
		j["type"] = "Vector3";
		j["value"] = { vv.x, vv.y, vv.z };
		break;
	}
	case Value::Type::Color: {
		const auto& vv = std::get<Vector4>(v.data);
		j["type"] = "Color";
		j["value"] = { vv.x, vv.y, vv.z, vv.w };
		break;
	}
	case Value::Type::Texture:
		j["type"] = "Texture";
		j["value"] = std::get<std::string>(v.data);
		break;
	}
	return j;
}

json NodeGraph::SerializeNode(const MyNode& node) {
	json j;
	j["id"] = static_cast<uintptr_t>(node.id);
	j["name"] = node.name;
	j["nodeType"] = NodeTypeToString(node.type);

	// values を配列で
	j["values"] = json::array();
	for (const auto& v : node.values) j["values"].push_back(SerializeValue(v));

	// Texture 型のときのみ texName を出す
	if (node.type == MyNode::NodeType::Texture) {
		j["texName"] = node.texName;
	}

	// ピン情報から繋がっているノード探して再帰的に保存していく
	if (node.inputs.size() != 0) {
		for (int i = 0; i < node.inputs.size(); i++) {
			j["child"][i] = nullptr;
			if (node.inputs[i].isLinked == true) {
				const Link* pLink = nullptr;
				for (const Link& link : links) {
					if (link.endPinId == node.inputs[i].id) {
						pLink = &link;
						break; // 見つかったら即終了
					}
				}
				if (!pLink)
					continue;
				MyNode* srcNode = FindNodeByPinId(pLink->startPinId);
				if (!srcNode)
					continue;
				j["child"][i] = SerializeNode(*srcNode); // ["child"][i]がなかった場合はnullとして保存される
			}
		}
	} // もし["child"]が何もなければ["child"]欄ができない->変更:Inputがあれば繋がっていない場合はnull返しておく

	return j;
}

ed::NodeId NodeGraph::DeserializeNodeData(const std::string& filePath) {
	json data = JsonSerializer::DeserializeJsonData(filePath);
	nodes.clear();
	links.clear();

	if (!data.contains("Nodes") || !data["Nodes"].is_array()) {
		return materialNodeId_;
	}

	for (const auto& nodeData : data["Nodes"]) {
		MyNode node = DeserializeNode(nodeData);
		nodes.push_back(node);
	}

	return materialNodeId_;
}

MyNode NodeGraph::DeserializeNode(const json& j) {
	MyNode node;

	node.CreateNode(StringToNodeType(j.value("nodeType", "Unknown")));

	if (node.type == MyNode::NodeType::Material) {
		materialNodeId_ = node.id;
	}

	// values の復元
	if (j.contains("values") && j["values"].is_array()) {
		node.values.clear();
		node.values.resize(j["values"].size());
		for (size_t i = 0; i < j["values"].size(); ++i) {
			node.values[i] = DeserializeValue(j["values"][i]);
		}
	}

	// Textureノード専用
	if (j.contains("texName")) {
		node.texName = j["texName"].get<std::string>();
	}

	// child ノード（入力側に繋がっているノード）を再帰的に復元
	if (j.contains("child") && j["child"].is_array()) {
		for (int i = 0; i < j["child"].size(); i++) {
			const auto& value = j["child"][i];
			if (value.is_null()) {
				continue; // null の場合はスキップ
			}
			MyNode childNode = DeserializeNode(value);
			nodes.push_back(childNode);

			ImGuiManager::GetInstance()->CreateLink(links, node.inputs[i], childNode.outputs[0]);
		}
	}

	return node;
}

Value NodeGraph::DeserializeValue(const json& j) {
	Value v;
	std::string type = j.value("type", "None");

	if (type == "Int") {
		v.type = Value::Type::Int;
		v.data = j["value"].get<int>();
	} else if (type == "Float") {
		v.type = Value::Type::Float;
		v.data = j["value"].get<float>();
	} else if (type == "Vector2") {
		v.type = Value::Type::Vector2;
		auto arr = j["value"];
		v.data = Vector2{ arr[0], arr[1] };
	} else if (type == "Vector3") {
		v.type = Value::Type::Vector3;
		auto arr = j["value"];
		v.data = Vector3{ arr[0], arr[1], arr[2] };
	} else if (type == "Color") {
		v.type = Value::Type::Color;
		auto arr = j["value"];
		v.data = Vector4{ arr[0], arr[1], arr[2], arr[3] };
	} else if (type == "Texture") {
		v.type = Value::Type::Texture;
		v.data = j["value"].get<std::string>();
	} else {
		v.type = Value::Type::None;
	}

	return v;
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
		inputs.push_back({ ImGuiManager::GetInstance()->GeneratePinId(), false, Pin::Type::Input, PinType::Vector2 });
		values.push_back(Value("white2x2.png"));
		values.push_back(Value(Vector4(1.0f, 1.0f, 1.0f, 1.0f)));
		values.push_back(Value(Vector2(0.0f, 0.0f)));
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
	case MyNode::NodeType::Vector2:

		name = "Vec2Node";
		type = MyNode::NodeType::Vector2;
		inputs.push_back({ ImGuiManager::GetInstance()->GeneratePinId(), false, Pin::Type::Input, PinType::Float });
		inputs.push_back({ ImGuiManager::GetInstance()->GeneratePinId(), false, Pin::Type::Input, PinType::Float });
		outputs.push_back({ ImGuiManager::GetInstance()->GeneratePinId(), false, Pin::Type::Output, PinType::Vector2 });
		values.push_back(Value(Vector2(0.0f, 0.0f)));
		evaluator = [](const std::vector<Value>& inputs) {
			return !inputs.empty() ? inputs[0] : Value();
			};

		break;
	default:
		break;
	}
}
#endif // _DEBUG
