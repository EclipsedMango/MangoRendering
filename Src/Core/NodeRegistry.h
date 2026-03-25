
#ifndef MANGORENDERING_NODEREGISTRY_H
#define MANGORENDERING_NODEREGISTRY_H

#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

class Node3d;

class NodeRegistry {
public:
    static void Register(const std::string& type, std::function<Node3d*()> factory) {
        GetMap()[type] = std::move(factory);
    }

    static Node3d* Create(const std::string& type) {
        auto& map = GetMap();
        auto it = map.find(type);
        if (it == map.end()) throw std::runtime_error("Unknown node type: " + type);
        return it->second();
    }

private:
    static std::unordered_map<std::string, std::function<Node3d*()>>& GetMap() {
        static std::unordered_map<std::string, std::function<Node3d*()>> s_map;
        return s_map;
    }
};

#define REGISTER_NODE_TYPE(Type) \
    static inline bool _registered_##Type = []() { \
        NodeRegistry::Register(#Type, []() -> Node3d* { return new Type(); }); \
        return true; \
    }();

#endif //MANGORENDERING_NODEREGISTRY_H