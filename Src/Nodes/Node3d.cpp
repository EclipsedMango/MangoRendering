
#include "Node3d.h"

Node3d::Node3d() = default;

Node3d::~Node3d() {
    for (const auto* child : m_children) {
        delete child;
    }
}

void Node3d::AddChild(Node3d* child) {
    child->m_parent = this;
    m_children.push_back(child);
}

void Node3d::RemoveChild(Node3d* child) {
    child->m_parent = nullptr;
    std::erase(m_children, child);
}

void Node3d::PhysicsProcess(float deltaTime) {
    // override in subclasses
}

void Node3d::Process() {
    // override in subclasses
}

void Node3d::SetRoot() {
    m_is_root = true;
}