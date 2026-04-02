
#ifndef MANGORENDERING_INSPECTORPANEL_H
#define MANGORENDERING_INSPECTORPANEL_H

#include <memory>
#include <string>
#include "Core/PropertyHolder.h"

class Editor;
class Node3d;
class Material;
class Texture;
class PortalNode3d;

class InspectorPanel {
public:
    explicit InspectorPanel(Editor* editor);
    ~InspectorPanel() = default;

    void DrawInspector(Node3d* selectedNode);
    void OpenTexturePreview(const std::shared_ptr<Texture>& tex, const char* label);

private:
    void DrawProperties(PropertyHolder* holder);
    void DrawPortalProperties(PortalNode3d *portal) const;
    void DrawPropertyValue(const std::string& name, PropertyHolder* holder);

    void DrawTexturePreviewPopup();
    std::shared_ptr<Texture> GetCachedTexture(const std::string& path);

    Editor* m_editor = nullptr;
    std::shared_ptr<Texture> m_previewTex;
    std::unordered_map<std::string, std::shared_ptr<Texture>> m_textureCache;
    std::string m_previewLabel;
};


#endif //MANGORENDERING_INSPECTORPANEL_H