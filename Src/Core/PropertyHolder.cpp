
#include "PropertyHolder.h"
#include "PropertyRegistry.h"

static inline bool sRegisteredPropertyHolder = [] {
    PropertyRegistry::Register("PropertyHolder", []() -> PropertyHolder* {
        return new PropertyHolder();
    });
    return true;
}();

PropertyValue PropertyHolder::DeserializePropertyValue(const fkyaml::node &val) {
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