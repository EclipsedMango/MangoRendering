
#include "SceneTreePanel.h"

#include <functional>
#include <memory>

#include "Editor.h"
#include "Core/Core.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "Nodes/CameraNode3d.h"
#include "Nodes/MeshNode3d.h"
#include "Nodes/Lights/DirectionalLightNode3d.h"
#include "Nodes/Lights/PointLightNode3d.h"
#include "Renderer/Meshes/PrimitiveMesh.h"

SceneTreePanel::SceneTreePanel(Editor* editor) : m_editor(editor) {}

void SceneTreePanel::DrawSceneTree(Node3d *node) {
    ImGui::Begin("Scene Tree");

    if (ImGui::Button("+ Add Node")) {
        ImGui::OpenPopup("AddNodePopup");
    }

    ImGui::Separator();

    // modal popup
    const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(260, 320), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("AddNodePopup", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar)) {
        ImGui::Text("Select a node type:");
        ImGui::Separator();
        ImGui::Spacing();

        struct NodeEntry { const char* label; const char* category; };
        static constexpr NodeEntry entries[] = {
            { "Node3d", "Base" },
            { "MeshNode3d", "3D" },
            { "CameraNode3d", "3D" },
            { "DirectionalLight", "Light" },
            { "PointLight", "Light" },
        };

        const char* lastCategory = nullptr;
        for (int i = 0; i < IM_ARRAYSIZE(entries); ++i) {
            // category header
            if (!lastCategory || strcmp(entries[i].category, lastCategory) != 0) {
                if (lastCategory) ImGui::Spacing();
                ImGui::TextDisabled("%s", entries[i].category);
                ImGui::Separator();
                lastCategory = entries[i].category;
            }

            if (ImGui::Selectable(entries[i].label)) {
                Node3d* root = m_editor->GetCore().GetScene();
                Node3d* created = nullptr;

                switch (i) {
                    case 0: {
                        created = new Node3d();
                        created->SetName("Node3d");
                        break;
                    }
                    case 1: {
                        created = new MeshNode3d(std::make_shared<CubeMesh>(), m_editor->GetCore().GetDefaultShader());
                        created->SetName("MeshNode3d");
                        break;
                    }
                    case 2: {
                        const glm::vec2 ws = m_editor->GetCore().GetActiveWindow()->GetSize();
                        const float aspect = (ws.y != 0.0f) ? (ws.x / ws.y) : 1.0f;
                        created = new CameraNode3d(glm::vec3(0, 0, 5), 75.0f, aspect);
                        created->SetName("CameraNode3d");
                        break;
                    }
                    case 3: {
                        created = new DirectionalLightNode3d({-64.0, 128.0, 0.0}, {1.0, 1.0, 1.0}, 0.25);
                        created->SetName("DirectionalLight");
                        break;
                    }
                    case 4: {
                        created = new PointLightNode3d({0, 0, 0}, {1.0, 1.0, 1.0}, 1.0);
                        created->SetName("PointLight");
                        break;
                    }
                    default: break;
                }

                if (created) {
                    root->AddChild(created);
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

    // Tree drawing
    std::function<void(Node3d*)> drawNode = [&](Node3d* n) {
        const ImGuiID sid = n->GetId();

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

        if (n->GetChildren().empty()) {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }

        if (m_selection.Contains(sid)) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        ImGui::SetNextItemSelectionUserData(sid);

        ImGui::PushID(static_cast<int>(sid));
        const bool open = ImGui::TreeNodeEx("##node", flags, "%s", n->GetName().c_str());
        ImGui::PopID();

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            m_lastSelectedNode = n;
        }

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
    ImGui::End();
}

void SceneTreePanel::DeleteSelectedNodes() {
    if (!m_lastSelectedNode || m_lastSelectedNode == m_editor->GetCore().GetScene()) {
        return;
    }

    std::vector<uint32_t> idsToDelete;
    idsToDelete.reserve(static_cast<size_t>(m_selection.Size));

    ImGuiID selId = 0;
    void* it = nullptr;
    while (m_selection.GetNextSelectedItem(&it, &selId)) {
        idsToDelete.push_back(static_cast<uint32_t>(selId));
    }

    for (const uint32_t id : idsToDelete) {
        Node3d* n = FindNodeById(m_editor->GetCore().GetScene(), id);
        if (!n || n == m_editor->GetCore().GetScene()) {
            continue;
        }

        if (n == m_lastSelectedNode) {
            m_lastSelectedNode = nullptr;
        }

        // TODO: add queue free
        // n->QueueFree();

        if (Node3d* p = n->GetParent()) p->RemoveChild(n);
        delete n;
    }

    m_selection.Clear();
}

void SceneTreePanel::DuplicateSelectedNodes() {
    if (!m_lastSelectedNode || m_selection.Size == 0) {
        return;
    }

    std::vector<uint32_t> idsToDuplicate;
    idsToDuplicate.reserve(static_cast<size_t>(m_selection.Size));

    ImGuiID selId = 0;
    void* it = nullptr;
    while (m_selection.GetNextSelectedItem(&it, &selId)) {
        idsToDuplicate.push_back(static_cast<uint32_t>(selId));
    }

    m_selection.Clear();

    std::vector<uint32_t> newIds;
    newIds.reserve(idsToDuplicate.size());

    for (const uint32_t id : idsToDuplicate) {
        const Node3d* n = FindNodeById(m_editor->GetCore().GetScene(), id);
        if (!n) {
            continue;
        }

        Node3d* parent = n->GetParent();
        if (!parent) {
            continue;
        }

        Node3d* duplicate = n->Clone();
        parent->AddChild(duplicate);
        newIds.push_back(duplicate->GetId());
    }

    for (const uint32_t newId : newIds) {
        m_selection.SetItemSelected(newId, true);
        m_lastSelectedNode = FindNodeById(m_editor->GetCore().GetScene(), newId);
        m_scrollToSelected = true;
    }
}

Node3d * SceneTreePanel::FindNodeById(Node3d *root, const uint32_t id) {
    if (!root) {
        return nullptr;
    }

    if (root->GetId() == id) {
        return root;
    }

    for (Node3d* c : root->GetChildren()) {
        if (Node3d* r = FindNodeById(c, id)) {
            return r;
        }
    }

    return nullptr;
}

int SceneTreePanel::CountNodesRecursive(const Node3d *root) {
    if (!root) {
        return 0;
    }

    int count = 1;
    for (const Node3d* c : root->GetChildren()) {
        count += CountNodesRecursive(c);
    }

    return count;
}
