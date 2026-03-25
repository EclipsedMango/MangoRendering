
#ifndef MANGORENDERING_PROPERTYREGISTRY_H
#define MANGORENDERING_PROPERTYREGISTRY_H

#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>

class PropertyHolder;

class PropertyRegistry {
public:
    static void Register(const std::string& type, std::function<PropertyHolder*()> factory) {
        GetMap()[type] = std::move(factory);
    }

    static PropertyHolder* Create(const std::string& type) {
        auto& map = GetMap();
        auto it = map.find(type);
        if (it == map.end()) throw std::runtime_error("Unknown property type: " + type);
        return it->second();
    }

private:
    static std::unordered_map<std::string, std::function<PropertyHolder*()>>& GetMap() {
        static std::unordered_map<std::string, std::function<PropertyHolder*()>> s_map;
        return s_map;
    }
};

#define REGISTER_PROPERTY_TYPE(Type) \
        static inline bool _registered_##Type = []() { \
        PropertyRegistry::Register(#Type, []() -> PropertyHolder* { return new Type(); }); \
        return true; \
    }();

#endif //MANGORENDERING_PROPERTYREGISTRY_H