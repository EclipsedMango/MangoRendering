
#ifndef MANGORENDERING_NODE3D_H
#define MANGORENDERING_NODE3D_H

#include <atomic>
#include <string>
#include <vector>

#include "Transform.h"

enum class NodeNotification;
class TreeListener;

class Node3d {
public:
    Node3d();
    virtual ~Node3d();

    void AddChild(Node3d* child);
    void RemoveChild(Node3d* child);

    void PropagateEnterTree(TreeListener* listener);
    void PropagateExitTree();

    virtual void PhysicsProcess(float deltaTime);
    virtual void Process(float deltaTime);

    void UpdateWorldTransform(const glm::mat4& parentWorld = glm::mat4(1.0f));

    void SetRoot();
    void SetName(const std::string& name) { m_name = name; }
    void SetVisible(const bool visible) { m_visible = visible; }
    void SetPosition(const glm::vec3 position) { m_transform.Position = position; }
    void SetRotation(const glm::quat rotation) { m_transform.Rotation = rotation; }
    void SetRotationEuler(const glm::vec3 degrees) { m_transform.SetEuler(degrees); }
    void SetScale(const glm::vec3 scale) { m_transform.Scale = scale; }

    [[nodiscard]] uint32_t GetId() const { return m_id; }

    [[nodiscard]] Node3d* GetParent() const { return m_parent; }
    [[nodiscard]] const std::vector<Node3d*>& GetChildren() const { return m_children; }

    [[nodiscard]] bool IsRoot() const { return m_is_root; }
    [[nodiscard]] std::string GetName() const { return m_name; }
    [[nodiscard]] bool IsVisible() const { return m_visible; }
    [[nodiscard]] glm::vec3 GetPosition() const { return m_transform.Position; }
    [[nodiscard]] glm::quat GetRotation() const { return m_transform.Rotation; }
    [[nodiscard]] glm::vec3 GetRotationEuler() const { return m_transform.GetEuler(); }
    [[nodiscard]] glm::vec3 GetScale() const { return m_transform.Scale; }
    [[nodiscard]] glm::mat4 GetModelMatrix() const { return m_transform.GetModelMatrix(); }
    [[nodiscard]] Transform GetTransform() const { return m_transform; }
    [[nodiscard]] glm::mat4 GetWorldMatrix() const { return m_worldMatrix; }

private:
    uint32_t m_id;
    inline static std::atomic<uint32_t> s_nextId = 1;

    std::string m_name = "Node3d";
    TreeListener* m_treeListener = nullptr;

    Node3d* m_parent = nullptr;
    std::vector<Node3d*> m_children;

    glm::mat4 m_worldMatrix = glm::mat4(1.0f);
    Transform m_transform;
    bool m_visible = true;
    bool m_is_root = false;
};


#endif //MANGORENDERING_NODE3D_H