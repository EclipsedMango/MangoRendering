
#ifndef MANGORENDERING_SCENETREEPANEL_H
#define MANGORENDERING_SCENETREEPANEL_H

#include <cstdint>
#include "imgui.h"

class Core;
class Node3d;
class Editor;

class SceneTreePanel {
public:
    explicit SceneTreePanel(Editor* editor);
    ~SceneTreePanel() = default;

    [[nodiscard]] Node3d* GetSelectedNode() const { return m_lastSelectedNode; };

    void DrawSceneTree(Node3d* node);
    void DeleteSelectedNodes();
    void DuplicateSelectedNodes();

private:
    [[nodiscard]] static Node3d* FindNodeById(Node3d* root, uint32_t id);
    [[nodiscard]] static int CountNodesRecursive(const Node3d* root);

    Editor* m_editor = nullptr;

    bool m_scrollToSelected = false;
    ImGuiSelectionBasicStorage m_selection;
    Node3d* m_lastSelectedNode = nullptr;
};


#endif //MANGORENDERING_SCENETREEPANEL_H