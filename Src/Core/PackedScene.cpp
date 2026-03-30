
#include "PackedScene.h"
#include "Nodes/Node3d.h"

#include <fstream>
#include <iostream>
#include <unordered_set>
#include <fkYAML/node.hpp>

#include "Nodes/PortalNode3d.h"

PackedNode::PackedNode(const Node3d *node) {
    std::function<PropertyValue(PropertyValue)> captureValue = [&](PropertyValue val) -> PropertyValue {
        if (std::holds_alternative<std::shared_ptr<PropertyHolder>>(val)) {
            const auto& holder = std::get<std::shared_ptr<PropertyHolder>>(val);
            auto newHolder = std::make_shared<PropertyHolder>();
            newHolder->SetPropertyHolderType(holder->GetPropertyHolderType());
            for (const auto& [k, p] : holder->GetProperties()) {
                PropertyValue propVal = captureValue(p.getter());
                newHolder->AddProperty(k, [propVal]{ return propVal; }, {});
            }
            return newHolder;
        }
        return val;
    };

    for (const auto& [key, prop] : node->GetProperties()) {
        const PropertyValue val = prop.getter();
        properties[key] = captureValue(val);
    }

    for (const auto& child : node->GetChildren()) {
        children.emplace_back(child);
    }

    type = node->GetNodeType();
}

PackedNode::PackedNode(fkyaml::node node) {
    type = node["node_type"].get_value<std::string>();

    for (auto& [key, val] : node["properties"].get_value<fkyaml::ordered_map<std::string, fkyaml::node>>()) {
        properties[key] = DeserializePropertyValue(val);
    }

    if (node.contains("children") && node["children"].is_sequence()) {
        for (auto& child : node["children"]) {
            children.emplace_back(child);
        }
    }
}

PropertyValue PackedNode::DeserializePropertyValue(const fkyaml::node &val) {
    if (val.is_null()) {
        return std::string(""); // null yaml value = empty string
    }

    if (val.is_boolean()) {
        return val.get_value<bool>();
    }

    if (val.is_integer()) {
        return val.get_value<int>();
    }

    if (val.is_float_number()) {
        return static_cast<float>(val.get_value<double>());
    }

    if (val.is_string()) {
        return val.get_value<std::string>();
    }

    if (val.is_mapping() && val.contains("z")) {
        auto getFloat = [](const fkyaml::node& n) -> float {
            if (n.is_null()) return 0.0f;
            if (n.is_integer()) return static_cast<float>(n.get_value<int>());
            return static_cast<float>(n.get_value<double>());
        };

        return glm::vec3(getFloat(val["x"]), getFloat(val["y"]), getFloat(val["z"]));
    }

    if (val.is_mapping() && val.contains("x") && val.contains("y")) {
        return glm::vec2(
            val["x"].get_value<float>(),
            val["y"].get_value<float>()
        );
    }

    if (val.is_mapping() && val.contains("property_type")) {
        const std::string type = val["property_type"].get_value<std::string>();

        std::shared_ptr<PropertyHolder> holder;
        try {
            holder = std::shared_ptr<PropertyHolder>(PropertyRegistry::Create(type));
        } catch (...) {
            holder = std::make_shared<PropertyHolder>();
        }

        for (auto& [subKey, subVal] : val.get_value<fkyaml::ordered_map<std::string, fkyaml::node>>()) {
            if (subKey == "property_type") continue;
            PropertyValue deserialized = DeserializePropertyValue(subVal);
            try {
                holder->Set(subKey, deserialized);
            } catch (const std::runtime_error&) {
                holder->AddProperty(subKey, [deserialized] { return deserialized; }, {});
            }
        }
        return holder;
    }

    if (val.is_mapping()) {
        auto holder = std::make_shared<PropertyHolder>();
        for (auto& [subKey, subVal] : val.get_value<fkyaml::ordered_map<std::string, fkyaml::node>>()) {
            PropertyValue captured = DeserializePropertyValue(subVal);
            holder->AddProperty(subKey, [captured] { return captured; }, {});
        }
        return holder;
    }

    return 0;
}

PackedScene::PackedScene(const Node3d *node) : m_node(PackedNode(node)) {}

PackedScene PackedScene::LoadFromFile(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("PackedScene: Cannot open file: " + path);
    }

    auto yaml = fkyaml::node::deserialize(file);
    PackedScene scene;
    scene.m_node = PackedNode(yaml);
    return scene;
}

void PackedScene::SaveToFile(const std::string &path) {
    std::ofstream file(path);
    file << fkyaml::node::serialize(FromPackedNode(*m_node));
}

std::unique_ptr<Node3d> PackedScene::Instantiate() const {
    if (!m_node)
        throw std::runtime_error("PackedScene: Cannot instantiate scene without a node");

    auto root = InstantiateNode(*m_node);
    RelinkPortals(root.get());
    return root;
}

std::unique_ptr<Node3d> PackedScene::InstantiateNode(const PackedNode &packedNode) {
    std::unique_ptr<Node3d> node(NodeRegistry::Create(packedNode.type));

    try {
        for (const auto& [key, val] : packedNode.properties) {
            node->Set(key, val);
        }
    } catch (const std::exception& e) {
        std::cerr << "[PackedScene Warning] Node '" << packedNode.type << "': " << e.what() << std::endl;
    }

    for (const auto& child : packedNode.children) {
        node->AddChild(InstantiateNode(child));
    }

    return node;
}

void PackedScene::RelinkPortals(Node3d *root) {
    std::unordered_map<std::string, PortalNode3d*> portalMap;
    std::function<void(Node3d*)> collect = [&](Node3d* node) {
        if (auto* p = dynamic_cast<PortalNode3d*>(node)) portalMap[p->GetName()] = p;
        for (Node3d* child : node->GetChildren()) {
            collect(child);
        }
    };
    collect(root);

    std::unordered_set<PortalNode3d*> linked;
    std::function<void(Node3d*)> relink = [&](Node3d* node) {
        if (auto* p = dynamic_cast<PortalNode3d*>(node)) {
            if (!linked.contains(p)) {
                const std::string partnerName = p->Get<std::string>("linked_portal_name");
                if (!partnerName.empty()) {
                    auto it = portalMap.find(partnerName);
                    if (it != portalMap.end()) {
                        PortalNode3d::LinkPair(p, it->second);
                        linked.insert(p);
                        linked.insert(it->second);
                    }
                }
            }
        }
        for (Node3d* child : node->GetChildren()) {
            relink(child);
        }
    };
    relink(root);
}

fkyaml::node PackedScene::FromPackedNode(const PackedNode& packedNode) {
    fkyaml::node properties = fkyaml::node::mapping();
    for (const auto& [key, prop] : packedNode.properties) {
        properties[key] = SerializeProperty(prop);
    }

    std::vector<fkyaml::node> children {};
    for (const auto& child : packedNode.children) {
        children.emplace_back(FromPackedNode(child));
    }

    return fkyaml::ordered_map<std::string, fkyaml::node> {{"properties", properties}, {"children", children}, {"node_type", packedNode.type}};
}

fkyaml::node PackedScene::SerializeProperty(PropertyValue property) {
    fkyaml::node ret;
    std::visit([&]<typename T0>(T0&& val) {
        using T = std::decay_t<T0>;

        if constexpr (std::is_same_v<T, float>) {
            ret = static_cast<float>(val);
        }

        if constexpr (std::is_same_v<T, int>) {
            ret = static_cast<int>(val);
        }

        if constexpr (std::is_same_v<T, bool>) {
            ret = static_cast<bool>(val);
        }

        if constexpr (std::is_same_v<T, std::string>) {
            ret = static_cast<std::string>(val);
        }

        if constexpr (std::is_same_v<T, glm::vec2>) {
            glm::vec2 v = val;
            ret = fkyaml::node { {"x", v.x}, {"y", v.y} };
        }

        if constexpr (std::is_same_v<T, glm::vec3>) {
            glm::vec3 v = val;
            ret = fkyaml::node { {"x", v.x}, {"y", v.y}, {"z", v.z} };
        }

        if constexpr (std::is_same_v<T, std::shared_ptr<PropertyHolder>>) {
            fkyaml::node properties = fkyaml::node::mapping();
            properties["property_type"] = val->GetPropertyHolderType();
            for (const auto& [key, prop] : val->GetProperties()) {
                properties[key] = SerializeProperty(prop.getter());
            }
            ret = properties;
        }
    }, property);

    return ret;
}
