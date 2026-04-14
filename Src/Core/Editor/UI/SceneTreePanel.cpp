
#include "SceneTreePanel.h"

#include <functional>
#include <memory>
#include <unordered_set>

#include "../Editor.h"
#include "Core/Core.h"
#include "Core/ResourceManager.h"
#include "glm/mat4x4.hpp"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "Nodes/CameraNode3d.h"
#include "Nodes/AnimationControllerNode3d.h"
#include "Nodes/MeshNode3d.h"
#include "Nodes/PortalNode3d.h"
#include "Nodes/Lights/DirectionalLightNode3d.h"
#include "Nodes/Lights/PointLightNode3d.h"
#include "Renderer/Meshes/PrimitiveMesh.h"

SceneTreePanel::SceneTreePanel(Editor* editor) : m_editor(editor) {}

static std::vector<uint32_t> GetSelectedIds(ImGuiSelectionBasicStorage& selection) {
    std::vector<uint32_t> ids;
    ids.reserve(static_cast<size_t>(selection.Size));
    ImGuiID selId = 0;
    void* it = nullptr;
    while (selection.GetNextSelectedItem(&it, &selId)) {
        ids.push_back(static_cast<uint32_t>(selId));
    }
    return ids;
}

void SceneTreePanel::ClearSelection() {
    m_selection.Clear();
    m_lastSelectedNode = nullptr;
    m_renamingNode = nullptr;
}

std::vector<Node3d*> SceneTreePanel::GetSelectedNodes() {
    const std::vector<uint32_t> idsToDuplicate = GetSelectedIds(m_selection);

    Node3d* activeScene = m_editor->GetState() == Editor::State::Playing ? m_editor->GetCore().GetScene() : m_editor->GetActiveViewport()->GetScene();
    std::vector<Node3d*> selectedNodes;

    if (!activeScene) return selectedNodes;

    for (const auto id : idsToDuplicate) {
        Node3d* selectedNode = FindNodeById(activeScene, id);
        if (!selectedNode) continue;
        selectedNodes.push_back(selectedNode);
    }

    return selectedNodes;
}

void SceneTreePanel::BeginRename(Node3d *node) {
    m_renamingNode = node;
    strncpy(m_renameBuf, node->GetName().c_str(), sizeof(m_renameBuf));
    m_renameBuf[sizeof(m_renameBuf) - 1] = '\0';
}

void SceneTreePanel::DrawSceneTree(Node3d *node) {
    ImGui::Begin("Scene Tree");
    m_hoveringSceneTree = ImGui::IsWindowHovered();

    if (!node) {
        ImGui::TextDisabled("No active scene.");
        ImGui::End();
        return;
    }

    ImGui::PushID(node);

    if (ImGui::Button("+ Add Node")) {
        ImGui::OpenPopup("AddNodePopup");
    }

    ImGui::Separator();

    // modal popup
    const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(300, 420), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("AddNodePopup", nullptr, ImGuiWindowFlags_NoResize)) {
        ImGui::Text("Select a node type:");
        ImGui::Separator();
        ImGui::Spacing();

        struct NodeEntry { const char* label; const char* category; };
        static constexpr NodeEntry entries[] = {
            { "Node3d", "Base" },
            { "MeshNode3d", "3D" },
            { "CameraNode3d", "3D" },
            { "DirectionalLightNode3d", "Light" },
            { "PointLightNode3d", "Light" },
            { "PortalNode3d", "Portal" },
            { "AnimationControllerNode3d", "Animation" }
        };

        const char* lastCategory = nullptr;
        for (int i = 0; i < IM_ARRAYSIZE(entries); ++i) {
            if (!lastCategory || strcmp(entries[i].category, lastCategory) != 0) {
                if (lastCategory) ImGui::Spacing();
                ImGui::TextDisabled("%s", entries[i].category);
                ImGui::Separator();
                lastCategory = entries[i].category;
            }

            if (ImGui::Selectable(entries[i].label)) {
                Node3d* root = m_editor->GetState() == Editor::State::Playing ? m_editor->GetCore().GetScene() : m_editor->GetActiveViewport()->GetScene();
                Node3d* created = nullptr;

                switch (i) {
                    case 0: {
                        auto n = std::make_unique<Node3d>();
                        n->SetName("Node3d");
                        created = n.get();
                        root->AddChild(std::move(n));
                        break;
                    }
                    case 1: {
                        auto n = std::make_unique<MeshNode3d>(std::make_shared<CubeMesh>());
                        n->GetActiveMaterial()->SetShader(ResourceManager::Get().LoadShader("test", "test.vert", "test.frag"));
                        n->SetName("MeshNode3d");
                        created = n.get();
                        root->AddChild(std::move(n));
                        break;
                    }
                    case 2: {
                        const glm::vec2 ws = m_editor->GetCore().GetActiveWindow()->GetSize();
                        const float aspect = (ws.y != 0.0f) ? (ws.x / ws.y) : 1.0f;
                        auto n = std::make_unique<CameraNode3d>(glm::vec3(0, 0, 5), 75.0f, aspect);
                        n->SetName("CameraNode3d");
                        created = n.get();
                        root->AddChild(std::move(n));
                        break;
                    }
                    case 3: {
                        auto n = std::make_unique<DirectionalLightNode3d>(glm::vec3(-64.0f, 128.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), 0.25f);
                        n->SetName("DirectionalLightNode3d");
                        created = n.get();
                        root->AddChild(std::move(n));
                        break;
                    }
                    case 4: {
                        auto n = std::make_unique<PointLightNode3d>(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), 1.0f);
                        n->SetName("PointLightNode3d");
                        created = n.get();
                        root->AddChild(std::move(n));
                        break;
                    }
                    case 5: {
                        auto n = std::make_unique<PortalNode3d>();
                        n->SetName("PortalNode3d");
                        created = n.get();
                        root->AddChild(std::move(n));
                        break;
                    }
                    case 6: {
                        auto n = std::make_unique<AnimationControllerNode3d>();
                        n->SetName("AnimationControllerNode3d");
                        created = n.get();
                        root->AddChild(std::move(n));
                        break;
                    }
                    default: break;
                }

                if (created) {
                    m_lastSelectedNode = created;
                    m_scrollToSelected = true;
                    m_selection.Clear();
                    m_selection.SetItemSelected(created->GetId(), true);
                }

                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        if (ImGui::Button("Cancel", ImVec2(-1, 0))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    const int totalNodes = CountNodesRecursive(node);
    constexpr ImGuiMultiSelectFlags msFlags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_BoxSelect1d;

    ImGuiMultiSelectIO* msIO = ImGui::BeginMultiSelect(msFlags, m_selection.Size, totalNodes);
    m_selection.ApplyRequests(msIO);

    // tree drawing
    std::function<void(Node3d*)> drawNode = [&](Node3d* n) {
        const ImGuiID sid = n->GetId();

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;

        if (n == m_editor->GetCore().GetScene()) {
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
        }

        if (n->GetChildren().empty()) {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }

        if (m_selection.Contains(sid)) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        ImGui::SetNextItemSelectionUserData(sid);
        ImGui::PushID(static_cast<int>(sid));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.0f, 3.0f));

        bool open = false;

        if (m_renamingNode == n) {
            open = ImGui::TreeNodeEx("##node", flags | ImGuiTreeNodeFlags_AllowOverlap, "");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-FLT_MIN);

            if (!ImGui::IsItemActive()) ImGui::SetKeyboardFocusHere();

            ImGui::InputText("##rename", m_renameBuf, sizeof(m_renameBuf), ImGuiInputTextFlags_AutoSelectAll);

            if (ImGui::IsItemDeactivatedAfterEdit()) {
                if (m_renameBuf[0] != '\0') n->SetName(m_renameBuf);
                m_renamingNode = nullptr;
            } else if (ImGui::IsItemDeactivated()) {
                m_renamingNode = nullptr;
            }
        } else {
            open = ImGui::TreeNodeEx("##node", flags, "%s", n->GetName().c_str());

            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                m_lastSelectedNode = n;
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                BeginRename(n);
            }

            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Rename")) BeginRename(n);
                if (ImGui::MenuItem("Duplicate")) DuplicateSelectedNodes();
                if (ImGui::MenuItem("Delete")) {
                    ImGuiID id = 0;
                    void* it2 = nullptr;
                    while (m_selection.GetNextSelectedItem(&it2, &id)) {
                        m_pendingDeletes.push_back(id);
                    }
                }
                ImGui::Separator();
                if (ImGui::BeginMenu("Add Child")) {
                    if (ImGui::MenuItem("Node3D"))   { /* TODO */ }
                    if (ImGui::MenuItem("MeshNode")) { /* TODO */ }
                    ImGui::EndMenu();
                }
                ImGui::EndPopup();
            }

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoDisableHover)) {
                std::vector<uint32_t> dragIds;
                if (m_selection.Contains(sid)) {
                    dragIds = GetSelectedIds(m_selection);
                } else {
                    dragIds.push_back(sid);
                }

                ImGui::SetDragDropPayload("SCENE_TREE_NODE_REPARENT", dragIds.data(), dragIds.size() * sizeof(uint32_t));
                if (dragIds.size() > 1) {
                    ImGui::Text("Reparent %zu nodes", dragIds.size());
                } else {
                    ImGui::Text("Reparent %s", n->GetName().c_str());
                }
                ImGui::EndDragDropSource();
            }

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_TREE_NODE_REPARENT")) {
                    const auto* ids = static_cast<const uint32_t*>(payload->Data);
                    const int count = static_cast<int>(payload->DataSize / sizeof(uint32_t));
                    for (int k = 0; k < count; ++k) {
                        QueueReparentRequest(sid, ids[k]);
                    }
                }
                ImGui::EndDragDropTarget();
            }
        }

        ImGui::PopStyleVar();
        ImGui::PopID();

        if (n == m_lastSelectedNode && m_scrollToSelected) {
            ImGui::SetScrollHereY(0.5f);
            m_scrollToSelected = false;
        }

        if (open) {
            for (Node3d* child : n->GetChildren()) {
                drawNode(child);
            }
            ImGui::TreePop();
        }
    };

    drawNode(node);

    msIO = ImGui::EndMultiSelect();
    m_selection.ApplyRequests(msIO);

    ImGui::PopID();

    if (!m_pendingDeletes.empty()) {
        DeleteSelectedNodes();
        m_pendingDeletes.clear();
    }
    ProcessReparentRequests();

    ImGui::End();
}

void SceneTreePanel::DeleteSelectedNodes() {
    Node3d* activeScene = m_editor->GetState() == Editor::State::Playing ? m_editor->GetCore().GetScene() : m_editor->GetActiveViewport()->GetScene();

    if (!activeScene || !m_lastSelectedNode || m_lastSelectedNode == activeScene) {
        return;
    }

    const std::vector<uint32_t> idsToDelete = GetSelectedIds(m_selection);

    for (const uint32_t id : idsToDelete) {
        Node3d* n = FindNodeById(activeScene, id);
        if (!n || n == activeScene || n->GetNodeType() == std::string("SkyboxNode3d")) {
            continue;
        }

        if (n == m_lastSelectedNode) {
            m_lastSelectedNode = nullptr;
        }

        if (Node3d* p = n->GetParent()) {
            p->RemoveChild(n);
        }
    }

    m_selection.Clear();
}

void SceneTreePanel::DuplicateSelectedNodes() {
    if (!m_lastSelectedNode || m_selection.Size == 0) {
        return;
    }

    Node3d* activeScene = m_editor->GetState() == Editor::State::Playing ? m_editor->GetCore().GetScene() : m_editor->GetActiveViewport()->GetScene();
    if (!activeScene) return;

    const std::vector<uint32_t> idsToDuplicate = GetSelectedIds(m_selection);

    m_selection.Clear();

    std::vector<uint32_t> newIds;
    newIds.reserve(idsToDuplicate.size());

    for (const uint32_t id : idsToDuplicate) {
        Node3d* n = FindNodeById(activeScene, id);
        if (!n) continue;

        Node3d* parent = n->GetParent();
        if (!parent) continue;

        auto duplicate = n->Clone();
        Node3d* duplicateRaw = duplicate.get();
        parent->AddChild(std::move(duplicate));
        duplicateRaw->UpdateWorldTransform(parent->GetWorldMatrix());

        newIds.push_back(duplicateRaw->GetId());
    }

    for (const uint32_t newId : newIds) {
        m_selection.SetItemSelected(newId, true);
        m_lastSelectedNode = FindNodeById(activeScene, newId);
        m_scrollToSelected = true;
    }
}

Node3d* SceneTreePanel::FindNodeById(Node3d *root, const uint32_t id) {
    if (!root) return nullptr;
    if (root->GetId() == id) return root;

    for (Node3d* c : root->GetChildren()) {
        if (Node3d* r = FindNodeById(c, id)) return r;
    }

    return nullptr;
}

int SceneTreePanel::CountNodesRecursive(const Node3d *root) {
    if (!root) return 0;

    int count = 1;
    for (const Node3d* c : root->GetChildren()) {
        count += CountNodesRecursive(c);
    }

    return count;
}

bool SceneTreePanel::IsDescendantOf(const Node3d* node, const Node3d* potentialAncestor) {
    if (!node || !potentialAncestor) {
        return false;
    }

    const Node3d* current = node->GetParent();
    while (current) {
        if (current == potentialAncestor) {
            return true;
        }
        current = current->GetParent();
    }
    return false;
}

void SceneTreePanel::QueueReparentRequest(const uint32_t targetId, const uint32_t draggedId) {
    if (targetId == 0 || draggedId == 0) {
        return;
    }

    auto it = std::ranges::find_if(m_pendingReparents, [&](const ReparentRequest& req) {
        return req.targetId == targetId;
    });

    if (it == m_pendingReparents.end()) {
        ReparentRequest req;
        req.targetId = targetId;
        req.nodeIds.push_back(draggedId);
        m_pendingReparents.push_back(std::move(req));
        return;
    }

    if (std::ranges::find(it->nodeIds, draggedId) == it->nodeIds.end()) {
        it->nodeIds.push_back(draggedId);
    }
}

void SceneTreePanel::ProcessReparentRequests() {
    if (m_pendingReparents.empty()) {
        return;
    }

    Node3d* activeScene = m_editor->GetState() == Editor::State::Playing ? m_editor->GetCore().GetScene() : m_editor->GetActiveViewport()->GetScene();
    if (!activeScene) {
        m_pendingReparents.clear();
        return;
    }

    activeScene->UpdateWorldTransform();

    std::vector<uint32_t> movedIds;
    for (const auto& req : m_pendingReparents) {
        Node3d* target = FindNodeById(activeScene, req.targetId);
        if (!target) {
            continue;
        }

        std::unordered_set<uint32_t> selectedSet(req.nodeIds.begin(), req.nodeIds.end());
        std::vector<Node3d*> topLevelNodes;
        topLevelNodes.reserve(req.nodeIds.size());

        for (const uint32_t id : req.nodeIds) {
            Node3d* node = FindNodeById(activeScene, id);
            if (!node || node == activeScene || node == target) {
                continue;
            }

            if (IsDescendantOf(target, node)) {
                continue; // prevent cycles
            }

            const Node3d* parent = node->GetParent();
            bool ancestorSelected = false;
            while (parent) {
                if (selectedSet.contains(parent->GetId())) {
                    ancestorSelected = true;
                    break;
                }
                parent = parent->GetParent();
            }

            if (!ancestorSelected) {
                topLevelNodes.push_back(node);
            }
        }

        for (Node3d* node : topLevelNodes) {
            Node3d* oldParent = node->GetParent();
            if (!oldParent || oldParent == target) {
                continue;
            }

            const glm::mat4 oldWorld = node->GetWorldMatrix();
            std::unique_ptr<Node3d> detached = oldParent->DetachChild(node);
            if (!detached) {
                continue;
            }

            Node3d* raw = detached.get();
            target->AddChild(std::move(detached));
            const glm::mat4 newLocal = glm::inverse(target->GetWorldMatrix()) * oldWorld;
            raw->SetLocalTransform(newLocal);
            raw->UpdateWorldTransform(target->GetWorldMatrix());
            movedIds.push_back(raw->GetId());
        }
    }

    if (!movedIds.empty()) {
        m_selection.Clear();
        for (const uint32_t id : movedIds) {
            m_selection.SetItemSelected(id, true);
            m_lastSelectedNode = FindNodeById(activeScene, id);
        }
    }

    m_pendingReparents.clear();
}
