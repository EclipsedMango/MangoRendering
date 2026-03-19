
#ifndef MANGORENDERING_INSPECTORPANEL_H
#define MANGORENDERING_INSPECTORPANEL_H

#include <memory>
#include <string>

class PointLightNode3d;
class DirectionalLightNode3d;
class MeshNode3d;
class Editor;
class Node3d;
class Material;
class Texture;

class InspectorPanel {
public:
    explicit InspectorPanel(Editor* editor);
    ~InspectorPanel() = default;

    void DrawInspector(Node3d* selectedNode);
    void DrawMaterialInspector(Material& mat);
    void OpenTexturePreview(const Texture* tex, const char* label);

private:
    void DrawAlbedoOptions(Material& mat);
    void DrawRoughMetalOptions(Material& mat);
    void DrawNormalsOptions(Material& mat);
    void DrawAoOptions(Material& mat);
    void DrawEmissiveOptions(Material& mat);
    static void DrawUvOptions(Material& mat);
    static void DrawRenderFlags(Material& mat);

    static void DrawTransformOptions(Node3d* node);
    void DrawNodeSpecificOptions(Node3d* node);
    void DrawMeshOptions(MeshNode3d* node);
    static void DrawDirLightOptions(DirectionalLightNode3d* node);
    static void DrawPointLightOptions(PointLightNode3d* node);

    void DrawTexturePreviewPopup();
    void TextureSlot(const char* label, const std::shared_ptr<Texture>& tex);
    static void SectionHeader(const char* label);

    Editor* m_editor = nullptr;
    const Texture* m_previewTex = nullptr;
    std::string m_previewLabel;
};


#endif //MANGORENDERING_INSPECTORPANEL_H