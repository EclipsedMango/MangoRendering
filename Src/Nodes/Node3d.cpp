
#include "Node3d.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Core/ScriptManager.h"
#include "Core/TreeListener.h"

REGISTER_NODE_TYPE(Node3d)

Node3d::Node3d() : m_id(++s_nextId) {
    AddProperty("name",
        [this]{ return GetName(); },
        [this](const PropertyValue& val) { SetName(std::get<std::string>(val)); }
    );
    AddProperty("position",
        [this]{ return GetPosition(); },
        [this](const PropertyValue& val) { SetPosition(std::get<glm::vec3>(val)); }
    );
    AddProperty("rotation",
        [this]() -> PropertyValue { return GetRotationEuler(); },
        [this](const PropertyValue& v) { SetRotationEuler(std::get<glm::vec3>(v)); }
    );
    AddProperty("scale",
        [this]() -> PropertyValue { return m_scale; },
        [this](const PropertyValue& v) { SetScale(std::get<glm::vec3>(v)); }
    );
    AddProperty("visible",
        [this]() -> PropertyValue { return IsVisible(); },
        [this](const PropertyValue& v) { SetVisible(std::get<bool>(v)); }
    );
    AddProperty("process_enabled",
        [this]() -> PropertyValue { return IsProcessEnabled(); },
        [this](const PropertyValue& v) { SetProcessEnabled(std::get<bool>(v)); }
    );
    AddProperty("script",
        [this]() -> PropertyValue { return GetScriptPath(); },
        [this](const PropertyValue& v) { SetScript(std::get<std::string>(v)); }
    );
}

Node3d::~Node3d() {
    ScriptManager::Get().ClearScript(this);

    for (const auto* child : m_children) {
        delete child;
    }
}

void Node3d::AddChild(std::unique_ptr<Node3d> child) {
    Node3d* raw = child.release();
    raw->m_parent = this;
    m_children.push_back(raw);

    if (m_treeListener) {
        raw->PropagateEnterTree(m_treeListener);
    }
}

void Node3d::RemoveChild(Node3d* child) {
    child->m_parent = nullptr;
    std::erase(m_children, child);

    if (child->m_treeListener) {
        child->PropagateExitTree();
    }
}

std::unique_ptr<Node3d> Node3d::DetachChild(Node3d *child) {
    const auto it = std::ranges::find(m_children.begin(), m_children.end(), child);
    if (it == m_children.end()) {
        return nullptr;
    }

    if (child->m_treeListener) {
        child->PropagateExitTree();
    }

    child->m_parent = nullptr;
    m_children.erase(it);

    return std::unique_ptr<Node3d>(child);
}

void Node3d::PhysicsProcess(const float deltaTime) {
    ScriptManager::Get().CallPhysicsProcess(this, deltaTime);
    // override in subclasses
}

void Node3d::Process(const float deltaTime) {
    ScriptManager::Get().CallProcess(this, deltaTime);
    // override in subclasses
}

void Node3d::UpdateWorldTransform(const glm::mat4 &parentWorld) {
    m_worldMatrix = parentWorld * GetLocalMatrix();
    for (auto* child : m_children) {
        child->UpdateWorldTransform(m_worldMatrix);
    }
}

void Node3d::UpdateWorldTransformFromParent() {
    const glm::mat4 parentWorld = m_parent ? m_parent->m_worldMatrix : glm::mat4(1.0f);
    m_worldMatrix = parentWorld * GetLocalMatrix();
}

void Node3d::SetScript(const std::string &path) {
    m_scriptPath = path;
    ScriptManager::Get().SetScript(this, path);
}

void Node3d::SetRoot() {
    m_is_root = true;
}

void Node3d::SetLocalTransform(const glm::mat4 &mat) {
    m_position = glm::vec3(mat[3]);

    m_scale = {
        glm::length(glm::vec3(mat[0])),
        glm::length(glm::vec3(mat[1])),
        glm::length(glm::vec3(mat[2]))
    };

    constexpr float eps = 1e-4f;
    const glm::mat3 rotMat(
        m_scale.x > eps ? glm::vec3(mat[0]) / m_scale.x : glm::vec3(1,0,0),
        m_scale.y > eps ? glm::vec3(mat[1]) / m_scale.y : glm::vec3(0,1,0),
        m_scale.z > eps ? glm::vec3(mat[2]) / m_scale.z : glm::vec3(0,0,1)
    );

    m_rotation = glm::normalize(glm::quat_cast(rotMat));
    m_localDirty = true;
}

void Node3d::SetPosition(const glm::vec3 &position) {
    m_position = position;
    m_localDirty = true;
}

void Node3d::SetScale(const glm::vec3 &scale) {
    constexpr float eps = 1e-4f;
    m_scale = {
        std::abs(scale.x) < eps ? eps : scale.x,
        std::abs(scale.y) < eps ? eps : scale.y,
        std::abs(scale.z) < eps ? eps : scale.z
    };
    m_localDirty = true;
}

void Node3d::SetRotation(const glm::quat &rotation) {
    m_rotation = glm::normalize(rotation);
    m_localDirty = true;
}

void Node3d::SetRotationEuler(const glm::vec3 &degrees) {
    SetRotation(glm::quat(glm::radians(degrees)));
}

glm::vec3 Node3d::GetRotationEuler() const {
    return glm::degrees(glm::eulerAngles(GetRotation()));
}

glm::mat4 Node3d::GetLocalMatrix() {
    if (m_localDirty) {
        m_localMatrix = glm::translate(
            glm::mat4(1.0f), m_position) *
            glm::mat4_cast(m_rotation) *
            glm::scale(glm::mat4(1.0f),
            m_scale
        );
        m_localDirty = false;
    }
    return m_localMatrix;
}

void Node3d::CopyBaseStateTo(Node3d &clone) const {
    clone.m_name = m_name;
    clone.m_visible = m_visible;
    clone.m_processEnabled = m_processEnabled;
    clone.m_position = m_position;
    clone.m_rotation = m_rotation;
    clone.m_scale = m_scale;
    clone.m_localDirty = true;
    clone.m_is_root = m_is_root;

    if (!m_scriptPath.empty()) {
        clone.SetScript(m_scriptPath);
    } else {
        clone.m_scriptPath.clear();
    }
}

void Node3d::PropagateEnterTree(TreeListener *listener) {
    m_treeListener = listener;
    m_treeListener->Notification(this, NodeNotification::EnterTree);

    for (auto* child : m_children) {
        child->PropagateEnterTree(listener);
    }

    ScriptManager::Get().CallReady(this);

    // ready fires bottom-up after the whole subtree has entered
    m_treeListener->Notification(this, NodeNotification::Ready);
}

void Node3d::PropagateExitTree() {
    for (auto* child : m_children) {
        child->PropagateExitTree();
    }

    m_treeListener->Notification(this, NodeNotification::ExitTree);
    m_treeListener = nullptr;
}

[[nodiscard]] std::unique_ptr<Node3d> Node3d::Clone() {
    auto clone = std::make_unique<Node3d>();

    CopyBaseStateTo(*clone);

    for (const auto& child : m_children) {
        clone->AddChild(child->Clone());
    }

    return clone;
}
