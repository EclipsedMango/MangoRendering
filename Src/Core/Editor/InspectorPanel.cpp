
#include "InspectorPanel.h"

#include <iostream>
#include <memory>
#include <ostream>

#include "Editor.h"
#include "imgui.h"
#include "glm/glm.hpp"
#include "Nodes/MeshNode3d.h"
#include "Nodes/Node3d.h"
#include "Nodes/Lights/DirectionalLightNode3d.h"
#include "Nodes/Lights/PointLightNode3d.h"
#include "Renderer/Meshes/PrimitiveMesh.h"

InspectorPanel::InspectorPanel(Editor* editor) : m_editor(editor) {}

void InspectorPanel::DrawInspector(Node3d* selectedNode) {
    ImGui::Begin("Inspector");

    if (!selectedNode) {
        ImGui::TextDisabled("No node selected");
        ImGui::End();
        return;
    }

    char nameBuf[256];
    strncpy(nameBuf, selectedNode->GetName().c_str(), sizeof(nameBuf));
    nameBuf[sizeof(nameBuf) - 1] = '\0';
    if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) selectedNode->SetName(nameBuf);

    ImGui::Separator();
    DrawTransformOptions(selectedNode);
    DrawNodeSpecificOptions(selectedNode);
    ImGui::End();
}

void InspectorPanel::DrawMaterialInspector(Material& mat) {
    if (!ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    ImGui::PushID("MatInspector");
    DrawAlbedoOptions(mat);
    DrawRoughMetalOptions(mat);
    DrawNormalsOptions(mat);
    DrawAoOptions(mat);
    DrawEmissiveOptions(mat);
    DrawUvOptions(mat);
    DrawRenderFlags(mat);
}

void InspectorPanel::DrawAlbedoOptions(Material &mat) {
    SectionHeader("Albedo");
    glm::vec4 color = mat.GetAlbedoColor();
    if (ImGui::ColorEdit4("Color##albedo", &color.x, ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_AlphaBar)) {
        mat.SetAlbedoColor(color);
    }

    if (ImGui::BeginTable("##albedoTex", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerH)) {
        TextureSlot("Texture (Diffuse)", mat.GetDiffuse());
        ImGui::EndTable();
    }
}

void InspectorPanel::DrawRoughMetalOptions(Material &mat) {
    SectionHeader("Metallic / Roughness");
    float metallic  = mat.GetMetallicValue();
    float roughness = mat.GetRoughnessValue();

    ImGui::PushItemWidth(-1);

    ImGui::Text("Metallic");
    if (ImGui::SliderFloat("##metallic", &metallic, 0.0f, 1.0f, "%.3f")) {
        mat.SetMetallicValue(metallic);
    }

    ImGui::Text("Roughness");
    if (ImGui::SliderFloat("##roughness", &roughness, 0.0f, 1.0f, "%.3f")) {
        mat.SetRoughnessValue(roughness);
    }

    ImGui::PopItemWidth();

    if (ImGui::BeginTable("##mrTex", 2,
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerH)) {
        TextureSlot("Metallic/Roughness Map", mat.GetMetallic());
        ImGui::EndTable();
    }

    // show packed indicator
    if (mat.GetMetallic() && mat.GetRoughness() && mat.GetMetallic() == mat.GetRoughness()) {
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(120, 200, 255, 255));
        ImGui::TextUnformatted("  (Packed: G=Roughness, B=Metallic)");
        ImGui::PopStyleColor();
    }
}

void InspectorPanel::DrawNormalsOptions(Material &mat) {
    SectionHeader("Normal Map");
    if (ImGui::BeginTable("##normalTex", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerH)) {
        TextureSlot("Normal Map", mat.GetNormal());
        ImGui::EndTable();
    }

    float normalStr = mat.GetNormalStrength();
    ImGui::PushItemWidth(-1);
    ImGui::Text("Strength");
    if (ImGui::SliderFloat("##normalStr", &normalStr, 0.0f, 4.0f, "%.3f")) {
        mat.SetNormalStrength(normalStr);
    }

    ImGui::PopItemWidth();
}

void InspectorPanel::DrawAoOptions(Material &mat) {
    SectionHeader("Ambient Occlusion");
    if (ImGui::BeginTable("##aoTex", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerH)) {
        TextureSlot("AO Map", mat.GetAmbientOcclusion());
        ImGui::EndTable();
    }

    float aoStr = mat.GetAOStrength();
    ImGui::PushItemWidth(-1);
    ImGui::Text("Strength");
    if (ImGui::SliderFloat("##aoStr", &aoStr, 0.0f, 1.0f, "%.3f")) {
        mat.SetAOStrength(aoStr);
    }

    ImGui::PopItemWidth();
}

void InspectorPanel::DrawEmissiveOptions(Material &mat) {
    SectionHeader("Emission");
    glm::vec3 emColor = mat.GetEmissionColor();
    // HDR colour picker. no clamping so values can exceed 1 for bloom
    if (ImGui::ColorEdit3("Color##emission", &emColor.x, ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float)) {
        mat.SetEmissionColor(emColor);
    }

    float emStr = mat.GetEmissionStrength();
    ImGui::PushItemWidth(-1);
    ImGui::Text("Strength");
    if (ImGui::DragFloat("##emStr", &emStr, 0.01f, 0.0f, 64.0f, "%.3f")) {
        mat.SetEmissionStrength(emStr);
    }

    ImGui::PopItemWidth();

    if (ImGui::BeginTable("##emTex", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerH)) {
        TextureSlot("Emissive Map", mat.GetEmissive());
        ImGui::EndTable();
    }
}

void InspectorPanel::DrawUvOptions(Material &mat) {
    SectionHeader("UV");
    glm::vec2 uvScale  = mat.GetUVScale();
    glm::vec2 uvOffset = mat.GetUVOffset();

    ImGui::PushItemWidth(-1);
    ImGui::Text("Scale");
    if (ImGui::DragFloat2("##uvScale",  &uvScale.x,  0.01f, -100.0f, 100.0f, "%.3f")) {
        mat.SetUVScale(uvScale);
    }
    ImGui::Text("Offset");
    if (ImGui::DragFloat2("##uvOffset", &uvOffset.x, 0.01f, -100.0f, 100.0f, "%.3f")) {
        mat.SetUVOffset(uvOffset);
    }

    ImGui::PopItemWidth();
}

void InspectorPanel::DrawRenderFlags(Material &mat) {
    SectionHeader("Render Flags");
    bool doubleSided = mat.GetDoubleSided();
    if (ImGui::Checkbox("Double Sided", &doubleSided)) {
        mat.SetDoubleSided(doubleSided);
    }

    bool castShadows = mat.GetCastShadows();
    if (ImGui::Checkbox("Cast Shadows", &castShadows)) {
        mat.SetCastShadows(castShadows);
    }

    // blend mode combo
    const char* blendModes[] = { "Opaque", "Alpha Blend", "Alpha Scissor", "Additive" };
    int blendIndex = 0;
    switch (mat.GetBlendMode()) {
        case BlendMode::Opaque: blendIndex = 0; break;
        case BlendMode::AlphaBlend: blendIndex = 1; break;
        case BlendMode::AlphaScissor: blendIndex = 2; break;
        case BlendMode::Additive: blendIndex = 3; break;
    }

    ImGui::Text("Blend Mode");
    if (ImGui::Combo("##blendMode", &blendIndex, blendModes, IM_ARRAYSIZE(blendModes))) {
        switch (blendIndex) {
            case 0: mat.SetBlendMode(BlendMode::Opaque); break;
            case 1: mat.SetBlendMode(BlendMode::AlphaBlend); break;
            case 2: mat.SetBlendMode(BlendMode::AlphaScissor); break;
            case 3: mat.SetBlendMode(BlendMode::Additive); break;
            default: break;
        }
    }

    // alpha scissor threshold, only relevant when in scissor mode
    if (mat.GetBlendMode() == BlendMode::AlphaScissor) {
        float thresh = mat.GetAlphaScissorThreshold();
        ImGui::Text("Alpha Cutoff");
        if (ImGui::SliderFloat("##alphaScissor", &thresh, 0.0f, 1.0f, "%.3f")) {
            mat.SetAlphaScissorThreshold(thresh);
        }
    }

    ImGui::PopID();
}

void InspectorPanel::DrawTexturePreviewPopup() {
    if (!m_previewTex) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(520, 560), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    bool open = true;
    if (ImGui::Begin(m_previewLabel.c_str(), &open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize)) {

        const float panelW = ImGui::GetContentRegionAvail().x;

        static int  channel = 0; // 0=RGB, 1=R, 2=G, 3=B, 4=A
        static bool flipY = false;
        static float zoom = 1.0f;

        ImGui::SetNextItemWidth(160);
        ImGui::Combo("Channel##prev", &channel, "RGB\0Red\0Green\0Blue\0Alpha\0");
        ImGui::SameLine();
        ImGui::Checkbox("Flip Y", &flipY);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        ImGui::SliderFloat("Zoom", &zoom, 0.25f, 4.0f, "%.2fx");
        ImGui::SameLine();
        if (ImGui::SmallButton("Reset")) {
            zoom = 1.0f;
            channel = 0;
            flipY = false;
        }

        ImGui::Separator();

        const float imgSize = panelW * zoom;
        const ImVec2 imgSizeV = { imgSize, imgSize };

        const ImVec2 uv0 = flipY ? ImVec2(0, 1) : ImVec2(0, 0);
        const ImVec2 uv1 = flipY ? ImVec2(1, 0) : ImVec2(1, 1);

        // tint per channel selection
        ImVec4 tint = { 1, 1, 1, 1 };
        if (channel == 1) tint = { 1, 0, 0, 1 };
        else if (channel == 2) tint = { 0, 1, 0, 1 };
        else if (channel == 3) tint = { 0, 0, 1, 1 };
        else if (channel == 4) tint = { 1, 1, 1, 1 }; // alpha shown as grey via separate pass

        ImGui::BeginChild("##previewScroll", ImVec2(panelW, panelW), false, ImGuiWindowFlags_HorizontalScrollbar);

        const ImVec2 imgPos = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        constexpr float checkSize = 12.0f;
        for (float cy = 0; cy < imgSize; cy += checkSize) {
            for (float cx = 0; cx < imgSize; cx += checkSize) {
                const bool even = (static_cast<int>(cx / checkSize) + static_cast<int>(cy / checkSize)) % 2 == 0;
                dl->AddRectFilled(
                    { imgPos.x + cx, imgPos.y + cy },
                    { imgPos.x + cx + checkSize, imgPos.y + cy + checkSize },
                    even ? IM_COL32(80, 80, 80, 255) : IM_COL32(50, 50, 50, 255)
                );
            }
        }

        ImGui::Image(static_cast<ImTextureID>(static_cast<intptr_t>(m_previewTex->GetGLHandle())), imgSizeV, uv0, uv1, tint, ImVec4(0, 0, 0, 0));

        ImGui::EndChild();

        ImGui::Separator();
        ImGui::TextDisabled("%dx%d  |  %d channels  |  GL handle: %u",
            m_previewTex->GetWidth(),
            m_previewTex->GetHeight(),
            m_previewTex->GetChannels(),
            m_previewTex->GetGLHandle()
        );
    }

    ImGui::End();

    if (!open) {
        m_previewTex = nullptr;
    }
}

void InspectorPanel::OpenTexturePreview(const Texture *tex, const char *label) {
    m_previewTex = tex;
    m_previewLabel = std::string("Texture: ") + label + "###TexPreview";
}

void InspectorPanel::DrawTransformOptions(Node3d *node) {
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        glm::vec3 pos = node->GetPosition();
        glm::vec3 rot = node->GetRotationEuler();
        glm::vec3 scl = node->GetScale();

        if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) node->SetPosition(pos);
        if (ImGui::DragFloat3("Rotation", &rot.x, 0.5f)) node->SetRotationEuler(rot);
        if (ImGui::DragFloat3("Scale", &scl.x, 0.1f)) {
            scl.x = std::abs(scl.x) < 1e-4f ? 1e-4f : scl.x;
            scl.y = std::abs(scl.y) < 1e-4f ? 1e-4f : scl.y;
            scl.z = std::abs(scl.z) < 1e-4f ? 1e-4f : scl.z;
            node->SetScale(scl);
        }
    }
}

void InspectorPanel::DrawNodeSpecificOptions(Node3d *node) {
    if (const auto mesh = dynamic_cast<MeshNode3d*>(node)) {
        DrawMeshOptions(mesh);
    }

    if (const auto light = dynamic_cast<LightNode3d*>(node)) {
        if (const auto dr = dynamic_cast<DirectionalLightNode3d*>(light)) {
            DrawDirLightOptions(dr);
        }

        if (const auto pl = dynamic_cast<PointLightNode3d*>(light)) {
            DrawPointLightOptions(pl);
        }

        if (auto sp = dynamic_cast<SpotLight*>(light)) {
            std::cerr << "Spotlight not yet implemented" << std::endl;
        }
    }
}

void InspectorPanel::DrawMeshOptions(MeshNode3d *node) {
    if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen)) {
            const char* primitiveNames[] = { "Cube", "Plane", "Quad", "Sphere", "Cylinder", "Capsule" };

            // figure out which item is currently selected
            const auto meshPtr = node->GetMeshPtr();
            int current = -1;
            if (std::dynamic_pointer_cast<CubeMesh>(meshPtr)) current = 0;
            else if (std::dynamic_pointer_cast<PlaneMesh>(meshPtr)) current = 1;
            else if (std::dynamic_pointer_cast<QuadMesh>(meshPtr)) current = 2;
            else if (std::dynamic_pointer_cast<SphereMesh>(meshPtr)) current = 3;
            else if (std::dynamic_pointer_cast<CylinderMesh>(meshPtr)) current = 4;
            else if (std::dynamic_pointer_cast<CapsuleMesh>(meshPtr)) current = 5;

            int preview = current == -1 ? 0 : current;
            if (ImGui::Combo("Shape", &preview, primitiveNames, IM_ARRAYSIZE(primitiveNames))) {
                if (preview != current) {
                    switch (preview) {
                        case 0: node->SetMesh(std::make_shared<CubeMesh>()); break;
                        case 1: node->SetMesh(std::make_shared<PlaneMesh>()); break;
                        case 2: node->SetMesh(std::make_shared<QuadMesh>()); break;
                        case 3: node->SetMesh(std::make_shared<SphereMesh>()); break;
                        case 4: node->SetMesh(std::make_shared<CylinderMesh>()); break;
                        case 5: node->SetMesh(std::make_shared<CapsuleMesh>()); break;
                        default: node->SetMesh(std::make_shared<CubeMesh>()); break;
                    }
                }
            }

            // show editable params for the current primitive type
            if (current == 0) {
                if (auto* cube = dynamic_cast<CubeMesh*>(meshPtr.get())) {
                    float size = cube->GetSize();
                    if (ImGui::DragFloat("Size", &size, 0.01f, 0.01f, 100.0f)) {
                        cube->SetSize(size);
                    }
                }
            } else if (current == 1) {
                if (auto* plane = dynamic_cast<PlaneMesh*>(meshPtr.get())) {
                    float w = plane->GetWidth(), d = plane->GetDepth();
                    int sx = plane->GetSubdivisionsX(), sz = plane->GetSubdivisionsZ();
                    if (ImGui::DragFloat("Width", &w, 0.01f, 0.01f, 100.0f)) plane->SetWidth(w);
                    if (ImGui::DragFloat("Depth", &d, 0.01f, 0.01f, 100.0f)) plane->SetDepth(d);
                    if (ImGui::DragInt("SubdivX", &sx, 1, 1, 64)) plane->SetSubdivisionsX(sx);
                    if (ImGui::DragInt("SubdivZ", &sz, 1, 1, 64)) plane->SetSubdivisionsZ(sz);
                }
            } else if (current == 2) {
                if (auto* quad = dynamic_cast<QuadMesh*>(meshPtr.get())) {
                    float w = quad->GetWidth(), h = quad->GetHeight();
                    if (ImGui::DragFloat("Width", &w, 0.01f, 0.01f, 100.0f)) quad->SetWidth(w);
                    if (ImGui::DragFloat("Height", &h, 0.01f, 0.01f, 100.0f)) quad->SetHeight(h);
                }
            } else if (current == 3) {
                if (auto* sphere = dynamic_cast<SphereMesh*>(meshPtr.get())) {
                    float r = sphere->GetRadius();
                    int rings = sphere->GetRings(), sectors = sphere->GetSectors();
                    if (ImGui::DragFloat("Radius", &r, 0.01f, 0.01f, 100.0f)) sphere->SetRadius(r);
                    if (ImGui::DragInt("Rings", &rings, 1, 3, 64)) sphere->SetRings(rings);
                    if (ImGui::DragInt("Sectors", &sectors, 1, 3, 64)) sphere->SetSectors(sectors);
                }
            } else if (current == 4) {
                if (auto* cyl = dynamic_cast<CylinderMesh*>(meshPtr.get())) {
                    float r = cyl->GetRadius(), h = cyl->GetHeight();
                    int sectors = cyl->GetSectors();
                    if (ImGui::DragFloat("Radius", &r, 0.01f, 0.01f, 100.0f)) cyl->SetRadius(r);
                    if (ImGui::DragFloat("Height", &h, 0.01f, 0.01f, 100.0f)) cyl->SetHeight(h);
                    if (ImGui::DragInt("Sectors", &sectors, 1, 3, 64)) cyl->SetSectors(sectors);
                }
            } else if (current == 5) {
                if (auto* cap = dynamic_cast<CapsuleMesh*>(meshPtr.get())) {
                    float r = cap->GetRadius(), h = cap->GetHeight();
                    int rings = cap->GetRings(), sectors = cap->GetSectors();
                    if (ImGui::DragFloat("Radius", &r, 0.01f, 0.01f, 100.0f)) cap->SetRadius(r);
                    if (ImGui::DragFloat("Height", &h, 0.01f, 0.01f, 100.0f)) cap->SetHeight(h);
                    if (ImGui::DragInt("Rings", &rings, 1, 3, 64)) cap->SetRings(rings);
                    if (ImGui::DragInt("Sectors", &sectors, 1, 3, 64)) cap->SetSectors(sectors);
                }
            }
    }

    DrawMaterialInspector(node->GetActiveMaterial());
    DrawTexturePreviewPopup();

}

void InspectorPanel::DrawDirLightOptions(DirectionalLightNode3d *node) {
    if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        glm::vec4 color = glm::vec4(node->GetColor(), 1.0f);
        if (ImGui::ColorEdit4("Color", &color.x)) {
            node->SetColor(color);
        }

        float intensity = node->GetIntensity();
        if (ImGui::DragFloat("Intensity", &intensity, 0.1f)) {
            node->SetIntensity(intensity);
        }
    }
}

void InspectorPanel::DrawPointLightOptions(PointLightNode3d *node) {
    if (ImGui::CollapsingHeader("Point Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        glm::vec4 color = glm::vec4(node->GetColor(), 1.0f);
        if (ImGui::ColorEdit4("Color", &color.x)) node->SetColor(color);

        float rad = node->GetLight()->GetRadius();
        if (ImGui::DragFloat("Radius", &rad, 0.1f)) node->SetRadius(rad);

        float intensity = node->GetIntensity();
        if (ImGui::DragFloat("Intensity", &intensity, 0.1f)) node->SetIntensity(intensity);

        if (ImGui::CollapsingHeader("Attenuation", ImGuiTreeNodeFlags_DefaultOpen)) {
            float constant = node->GetLight()->GetConstant();
            float linear = node->GetLight()->GetLinear();
            float quad = node->GetLight()->GetQuadratic();
            if (ImGui::SliderFloat("Constant", &constant, 0.0f, 1.0f)) node->GetLight()->SetAttenuation(constant, linear, quad);
            if (ImGui::SliderFloat("Linear", &linear, 0.0f, 1.0f)) node->GetLight()->SetAttenuation(constant, linear, quad);
            if (ImGui::SliderFloat("Quad", &quad, 0.0f, 1.0f)) node->GetLight()->SetAttenuation(constant, linear, quad);
        }
    }
}

void InspectorPanel::SectionHeader(const char* label) {
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(180, 180, 180, 255));
    ImGui::SeparatorText(label);
    ImGui::PopStyleColor();
}

void InspectorPanel::TextureSlot(const char* label, const std::shared_ptr<Texture>& tex) {
    const bool has = tex != nullptr;

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);

    if (has) {
        ImGui::PushID(label);
        const bool clicked = ImGui::ImageButton( "##thumb", static_cast<ImTextureID>(static_cast<intptr_t>(tex->GetGLHandle())), ImVec2(32, 32));
        ImGui::PopID();

        if (clicked) {
            OpenTexturePreview(tex.get(), label);
        }

        ImGui::SameLine();
        ImGui::TextDisabled("%dx%d", tex->GetWidth(), tex->GetHeight());
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(50, 50, 50, 200));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(60, 60, 60, 200));
        ImGui::PushID(label);
        ImGui::Button("##empty", ImVec2(32, 32));
        ImGui::PopID();
        ImGui::PopStyleColor(2);
        ImGui::SameLine();
        ImGui::TextDisabled("None");
    }
}
