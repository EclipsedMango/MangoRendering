
#ifndef MANGORENDERING_PROPERTYHOLDER_H
#define MANGORENDERING_PROPERTYHOLDER_H

#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <variant>
#include <fkYAML/node.hpp>

#include "PropertyRegistry.h"
#include "glm/glm.hpp"

class PropertyHolder;
class Texture;

using PropertyValue = std::variant<
    int,
    float,
    bool,
    std::string,
    glm::vec2,
    glm::vec3,
    std::shared_ptr<PropertyHolder>
>;

struct Property {
    std::function<PropertyValue()> getter;
    std::function<void(const PropertyValue&)> setter;
};

class PropertyHolder {
public:
    virtual ~PropertyHolder() = default;

    static void RegisterBaseType() {
        PropertyRegistry::Register("PropertyHolder", []() -> PropertyHolder* {
            return new PropertyHolder();
        });
    }

    void AddProperty(const std::string& name, const std::function<PropertyValue()> &getter, const std::function<void(const PropertyValue&)> &setter) {
        if (!m_properties.contains(name)) {
            m_order.push_back(name);
        }

        m_properties[name] = Property{getter, setter};
    }

    void SetPropertyHolderType(const std::string& type) { m_holderType = type; }

    [[nodiscard]] const std::vector<std::string>& GetPropertyOrder() const { return m_order; }
    [[nodiscard]] const std::map<std::string, Property>& GetProperties() const { return m_properties; }
    [[nodiscard]] virtual std::string GetPropertyHolderType() const { return m_holderType; }

    [[nodiscard]] PropertyValue GetProperty(const std::string& name) const {
        const auto it = m_properties.find(name);
        if (it == m_properties.end()) {
            throw std::runtime_error("Property not found " + name);
        }

        return it->second.getter();
    }

    template<typename T>
    T Get(const std::string &name) const {
        PropertyValue value = GetProperty(name);
        if (!std::holds_alternative<T>(value)) {
            throw std::runtime_error("Property is wrong type, " + name);
        }

        return std::get<T>(value);
    }

    void Set(const std::string& name, const PropertyValue& value) {
        const auto it = m_properties.find(name);
        if (it == m_properties.end()) {
            throw std::runtime_error("Property not found " + name);
        }

        if (!it->second.setter) return;
        it->second.setter(value);
    }

    static PropertyValue DeserializePropertyValue(const fkyaml::node& val);

private:
    std::map<std::string, Property> m_properties;
    std::vector<std::string> m_order;
    std::string m_holderType = "PropertyHolder";
};

#endif //MANGORENDERING_PROPERTYHOLDER_H
