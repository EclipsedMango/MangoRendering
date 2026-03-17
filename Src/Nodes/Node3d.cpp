
#include "Node3d.h"

#include <iostream>

#include "Core/TreeListener.h"

Node3d::Node3d() : m_id(++s_nextId) {}

Node3d::~Node3d() {
    for (const auto* child : m_children) {
        delete child;
    }
}

void Node3d::AddChild(Node3d* child) {
    child->m_parent = this;
    m_children.push_back(child);

    if (m_treeListener) {
        child->PropagateEnterTree(m_treeListener);
    }
}

void Node3d::RemoveChild(Node3d* child) {
    child->m_parent = nullptr;
    std::erase(m_children, child);

    if (child->m_treeListener) {
        child->PropagateExitTree();
    }
}

void Node3d::PhysicsProcess(float deltaTime) {
    // override in subclasses
}

void Node3d::Process(float deltaTime) {
    // override in subclasses
}

void Node3d::UpdateWorldTransform(const glm::mat4 &parentWorld) {
    m_worldMatrix = parentWorld * GetModelMatrix();

    for (auto* child : m_children) {
        child->UpdateWorldTransform(m_worldMatrix);
    }
}

void Node3d::SetRoot() {
    m_is_root = true;
}

void Node3d::PropagateEnterTree(TreeListener *listener) {
    m_treeListener = listener;
    m_treeListener->Notification(this, NodeNotification::EnterTree);

    for (auto* child : m_children) {
        child->PropagateEnterTree(listener);
    }

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
