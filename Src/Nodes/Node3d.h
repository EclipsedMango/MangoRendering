
#ifndef MANGORENDERING_NODE3D_H
#define MANGORENDERING_NODE3D_H

#include <atomic>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Core/PropertyHolder.h"

enum class NodeNotification;
class TreeListener;

class Node3d : public PropertyHolder {
public:
    Node3d();
    virtual ~Node3d();

    void AddChild(Node3d* child);
    void RemoveChild(Node3d* child);

    void PropagateEnterTree(TreeListener* listener);
    void PropagateExitTree();

    [[nodiscard]] virtual Node3d* Clone() const;

    virtual void PhysicsProcess(float deltaTime);
    virtual void Process(float deltaTime);

    void UpdateWorldTransform(const glm::mat4& parentWorld = glm::mat4(1.0f));

    void SetRoot();
    void SetName(const std::string& name) { m_name = name; }
    void SetVisible(const bool visible) { m_visible = visible; }
    void SetLocalTransform(const glm::mat4& mat);
    void SetPosition(const glm::vec3& position);
    void SetScale(const glm::vec3& scale);
    void SetRotation(const glm::quat& rotation);
    void SetRotationEuler(const glm::vec3& degrees);

    [[nodiscard]] uint32_t GetId() const { return m_id; }

    [[nodiscard]] Node3d* GetParent() const { return m_parent; }
    [[nodiscard]] const std::vector<Node3d*>& GetChildren() const { return m_children; }

    [[nodiscard]] bool IsRoot() const { return m_is_root; }
    [[nodiscard]] std::string GetName() const { return m_name; }
    [[nodiscard]] bool IsVisible() const { return m_visible; }
    [[nodiscard]] glm::vec3 GetPosition() const { return m_position; }
    [[nodiscard]] glm::vec3 GetScale() const { return m_scale; }
    [[nodiscard]] glm::quat GetRotation() const { return m_rotation; }
    [[nodiscard]] glm::vec3 GetRotationEuler() const;
    [[nodiscard]] glm::mat4 GetLocalMatrix();
    [[nodiscard]] glm::mat4 GetWorldMatrix() const { return m_worldMatrix; }

private:
    uint32_t m_id;
    inline static std::atomic<uint32_t> s_nextId = 1;

    std::string m_name = "Node3d";
    TreeListener* m_treeListener = nullptr;

    Node3d* m_parent = nullptr;
    std::vector<Node3d*> m_children;

    glm::vec3 m_position = glm::vec3(0.0f);
    glm::quat m_rotation = glm::quat(1, 0, 0, 0);
    glm::vec3 m_scale = glm::vec3(1.0f);

    glm::mat4 m_localMatrix = glm::mat4(1.0f); // rebuilt from TRS on demand
    glm::mat4 m_worldMatrix = glm::mat4(1.0f);
    bool m_localDirty = false;

    bool m_visible = true;
    bool m_is_root = false;
};


#endif //MANGORENDERING_NODE3D_H