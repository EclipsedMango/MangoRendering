
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
};

class PackedScene {
public:
    explicit PackedScene(const Node3d* node);

    [[nodiscard]] static PackedScene LoadFromFile(const std::string& path);
    void SaveToFile(const std::string& path);

    [[nodiscard]] std::unique_ptr<Node3d> Instantiate() const;

private:
    PackedScene() = default;

    static std::unique_ptr<Node3d> InstantiateNode(const PackedNode& packedNode);

    static void RelinkPortals(Node3d* root);

    static fkyaml::node FromPackedNode(const PackedNode& packedNode);
    static fkyaml::node SerializeProperty(PropertyValue property);

    std::optional<PackedNode> m_node;
};


#endif //MANGORENDERING_PACKEDSCENE_H