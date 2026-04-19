#ifndef MANGORENDERING_COLLISIONSHAPE3D_H
#define MANGORENDERING_COLLISIONSHAPE3D_H

#include "Node3d.h"

class CollisionShape3d : public Node3d {
public:
    enum class ShapeType {
        Box,
        Sphere,
        Capsule
    };

    CollisionShape3d();
    ~CollisionShape3d() override = default;

    [[nodiscard]] std::unique_ptr<Node3d> Clone() override;

    void SetShapeType(ShapeType type);
    void SetShapeSize(const glm::vec3& size);

    [[nodiscard]] ShapeType GetShapeType() const { return m_shapeType; }
    [[nodiscard]] glm::vec3 GetShapeSize() const { return m_shapeSize; }

    [[nodiscard]] std::string GetNodeType() const override { return "CollisionShape3d"; }

    static std::string ShapeTypeToString(ShapeType type);
    static ShapeType ShapeTypeFromString(const std::string& type);

private:
    ShapeType m_shapeType = ShapeType::Box;
    glm::vec3 m_shapeSize = glm::vec3(0.5f);
};

#endif //MANGORENDERING_COLLISIONSHAPE3D_H
