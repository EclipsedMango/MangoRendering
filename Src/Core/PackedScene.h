
#ifndef MANGORENDERING_PACKEDSCENE_H
#define MANGORENDERING_PACKEDSCENE_H
#include <optional>
#include <string>
#include <fkYAML/node.hpp>

#include "PropertyHolder.h"

class PropertyHolder;
class Node3d;

struct PackedNode {
    explicit PackedNode(const Node3d* node);
    explicit PackedNode(fkyaml::node node);
    std::unordered_map<std::string, PropertyValue> properties;
    std::vector<PackedNode> children;
    std::string type;

private:
    static PropertyValue DeserializePropertyValue(const fkyaml::node& val);
};

class PackedScene {
public:
    explicit PackedScene(const Node3d* node);

    [[nodiscard]] static PackedScene LoadFromFile(const std::string& path);
    void SaveToFile(const std::string& path);

    Node3d* Instantiate() const;

private:
    PackedScene() = default;

    static Node3d* InstantiateNode(const PackedNode& packedNode);

    fkyaml::node FromPackedNode(const PackedNode& packedNode);
    fkyaml::node SerializeProperty(PropertyValue property);

    std::optional<PackedNode> m_node;
};


#endif //MANGORENDERING_PACKEDSCENE_H