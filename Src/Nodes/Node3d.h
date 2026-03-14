
#ifndef MANGORENDERING_NODE3D_H
#define MANGORENDERING_NODE3D_H

#include <vector>

#include "Transform.h"

class Node3d {
public:
    Node3d();
    ~Node3d();

    void AddChild(Node3d* child);
    void RemoveChild(Node3d* child);

    void PhysicsProcess(float deltaTime);
    void Process();

    void SetRoot();
    void SetVisible(const bool visible) { m_visible = visible; }
    void SetPosition(const glm::vec3 position) { m_transform.Position = position; }
    void SetRotation(const glm::vec3 rotation) { m_transform.Rotation = rotation; }
    void SetScale(const glm::vec3 scale) { m_transform.Scale = scale; }

    [[nodiscard]] Node3d* GetParent() const { return m_parent; }
    [[nodiscard]] const std::vector<Node3d*>& GetChildren() const { return m_children; }

    [[nodiscard]] bool IsRoot() const { return m_is_root; }
    [[nodiscard]] bool IsVisible() const { return m_visible; }
    [[nodiscard]] glm::vec3 GetPosition() const { return m_transform.Position; }
    [[nodiscard]] glm::vec3 GetRotation() const { return m_transform.Rotation; }
    [[nodiscard]] glm::vec3 GetScale() const { return m_transform.Scale; }
    [[nodiscard]] glm::mat4 GetModelMatrix() const { return m_transform.GetModelMatrix(); }
    [[nodiscard]] Transform GetTransform() const { return m_transform; }

private:
    Node3d* m_parent = nullptr;
    std::vector<Node3d*> m_children;

    Transform m_transform;
    bool m_visible = true;
    bool m_is_root = false;
};


#endif //MANGORENDERING_NODE3D_H