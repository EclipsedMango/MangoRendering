#include "CollisionShape3d.h"

#include <algorithm>

REGISTER_NODE_TYPE(CollisionShape3d)

CollisionShape3d::CollisionShape3d() {
    SetName("CollisionShape3d");

    AddProperty("shape_type",
        [this]() -> PropertyValue { return ShapeTypeToString(m_shapeType); },
        [this](const PropertyValue& v) { SetShapeType(ShapeTypeFromString(std::get<std::string>(v))); }
    );

    AddProperty("shape_size",
        [this]() -> PropertyValue { return m_shapeSize; },
        [this](const PropertyValue& v) { SetShapeSize(std::get<glm::vec3>(v)); }
    );
}

std::unique_ptr<Node3d> CollisionShape3d::Clone() {
    auto clone = std::make_unique<CollisionShape3d>();

    CopyBaseStateTo(*clone);
    clone->SetShapeType(m_shapeType);
    clone->SetShapeSize(m_shapeSize);

    for (Node3d* child : GetChildren()) {
        clone->AddChild(child->Clone());
    }

    return clone;
}

void CollisionShape3d::SetShapeType(const ShapeType type) {
    m_shapeType = type;
}

void CollisionShape3d::SetShapeSize(const glm::vec3& size) {
    m_shapeSize = glm::max(size, glm::vec3(0.01f));
}

std::string CollisionShape3d::ShapeTypeToString(const ShapeType type) {
    switch (type) {
        case ShapeType::Box: return "Box";
        case ShapeType::Sphere: return "Sphere";
        case ShapeType::Capsule: return "Capsule";
    }

    return "Box";
}

CollisionShape3d::ShapeType CollisionShape3d::ShapeTypeFromString(const std::string& type) {
    if (type == "Sphere") return ShapeType::Sphere;
    if (type == "Capsule") return ShapeType::Capsule;
    return ShapeType::Box;
}
