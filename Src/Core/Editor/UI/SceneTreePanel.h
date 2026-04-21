
#ifndef MANGORENDERING_SCENETREEPANEL_H
#define MANGORENDERING_SCENETREEPANEL_H

#include <cstdint>
#include <string>
#include <vector>

#include "imgui.h"

class Core;
class Node3d;
class Editor;

class SceneTreePanel {
public:
    explicit SceneTreePanel(Editor* editor);
    ~SceneTreePanel() = default;

    void ClearSelection();

    [[nodiscard]] Node3d* GetSelectedNode() const { return GetLastSelectedNode(); }
    [[nodiscard]] std::vector<Node3d*> GetSelectedNodes();
    [[nodiscard]] bool IsHoveringSceneTree() const { return m_hoveringSceneTree; }

    void BeginRename(Node3d* node);

    void DrawSceneTree(Node3d* node);
    void DeleteSelectedNodes();
    void DuplicateSelectedNodes();

private:
    struct ReparentRequest {
        std::vector<uint32_t> nodeIds;
        uint32_t targetId = 0;
    };

    [[nodiscard]] static Node3d* FindNodeById(Node3d* root, uint32_t id);
    [[nodiscard]] Node3d* GetLastSelectedNode() const;
    [[nodiscard]] static int CountNodesRecursive(const Node3d* root);
    static bool IsDescendantOf(const Node3d* node, const Node3d* potentialAncestor);
    void QueueReparentRequest(const uint32_t targetId, uint32_t draggedId);
    void ProcessReparentRequests();

    Editor* m_editor = nullptr;

    bool m_hoveringSceneTree = false;

    bool m_scrollToSelected = false;
    ImGuiSelectionBasicStorage m_selection;
    uint32_t m_lastSelectedId = 0;

    Node3d* m_renamingNode = nullptr;
    char m_renameBuf[256] = {};

    std::vector<uint32_t> m_pendingDeletes;
    std::vector<ReparentRequest> m_pendingReparents;
};


#endif //MANGORENDERING_SCENETREEPANEL_H
