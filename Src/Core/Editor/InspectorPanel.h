
#ifndef MANGORENDERING_INSPECTORPANEL_H
#define MANGORENDERING_INSPECTORPANEL_H

#include <memory>
#include <string>
#include "Core/PropertyHolder.h"

class Editor;
class Node3d;
class Material;
class Texture;

class InspectorPanel {
public:
    explicit InspectorPanel(Editor* editor);
    ~InspectorPanel() = default;

    void DrawInspector(Node3d* selectedNode);
    void OpenTexturePreview(const Texture* tex, const char* label);

private:
    void DrawProperties(PropertyHolder* holder);
    void DrawPropertyValue(const std::string& name, PropertyHolder* holder);

    void DrawTexturePreviewPopup();

    Editor* m_editor = nullptr;
    const Texture* m_previewTex = nullptr;
    std::string m_previewLabel;
};


#endif //MANGORENDERING_INSPECTORPANEL_H