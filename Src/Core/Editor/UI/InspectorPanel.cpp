#include "InspectorPanel.h"

#include <iostream>
#include <memory>

#include "../Editor.h"
#include "imgui.h"
#include "Core/Editor/EditorStyle.h"
#include "Core/ResourceManager.h"
#include "glm/glm.hpp"
#include "Nodes/Node3d.h"
#include "Nodes/PortalNode3d.h"

InspectorPanel::InspectorPanel(Editor* editor) : m_editor(editor) {}

static void SectionHeader(const char* label) {
    const auto& style = EditorStyle::Get();
    ImGui::Spacing();
    style.PushHeader();
    ImGui::SeparatorText(label);
    EditorStyle::PopHeader();
    ImGui::Spacing();
}

static void BeginPropertyTable() {
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4, 3));
    ImGui::BeginTable("##props", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV, ImVec2(0, 0));
    ImGui::TableSetupColumn("##label", ImGuiTableColumnFlags_WidthFixed, 130.0f);
    ImGui::TableSetupColumn("##value", ImGuiTableColumnFlags_WidthStretch);
}

static void EndPropertyTable() {
    ImGui::EndTable();
    ImGui::PopStyleVar();
}

static void PropertyLabel(const char* label) {
    const auto& style = EditorStyle::Get();

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    style.PushLabel();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    EditorStyle::PopLabel();
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-FLT_MIN);
}

void InspectorPanel::DrawInspector(Node3d* selectedNode) {
    const auto& style = EditorStyle::Get();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
    ImGui::Begin("Inspector");

    if (!selectedNode) {
        ImGui::Spacing();
        style.PushLabel();
        const float w = ImGui::GetContentRegionAvail().x;
        const char* msg = "No node selected";
        ImGui::SetCursorPosX((w - ImGui::CalcTextSize(msg).x) * 0.5f);
        ImGui::TextUnformatted(msg);
        EditorStyle::PopLabel();
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }

    style.PushAccentText();
    ImGui::TextUnformatted(selectedNode->GetName().c_str());
    EditorStyle::PopAccentText();

    ImGui::SameLine();

    style.PushLabel();
    ImGui::TextUnformatted("- Node3d");
    EditorStyle::PopLabel();

    ImGui::Separator();
    ImGui::Spacing();

    DrawProperties(selectedNode);
    if (auto* portal = dynamic_cast<PortalNode3d*>(selectedNode)) {
        DrawPortalProperties(portal);
    }

    DrawTexturePreviewPopup();

    ImGui::End();
    ImGui::PopStyleVar();
}

void InspectorPanel::DrawProperties(PropertyHolder* holder) {
    std::vector<std::string> flatProps;
    std::vector<std::string> subProps;

    for (const auto& name : holder->GetPropertyOrder()) {
        const PropertyValue v = holder->GetProperty(name);
        const bool isSub = std::holds_alternative<std::shared_ptr<PropertyHolder>>(v);
        (isSub ? subProps : flatProps).push_back(name);
    }

    if (!flatProps.empty()) {
        BeginPropertyTable();
        for (const auto& name : flatProps) {
            DrawPropertyValue(name, holder);
        }

        EndPropertyTable();
    }

    for (const auto& name : subProps) {
        DrawPropertyValue(name, holder);
    }
}

static void CollectPortals(Node3d* root, PortalNode3d* exclude, std::vector<PortalNode3d*>& out) {
    if (!root) return;
    if (auto* p = dynamic_cast<PortalNode3d*>(root); p && p != exclude) out.push_back(p);
    for (Node3d* child : root->GetChildren()) {
        CollectPortals(child, exclude, out);
    }
}

void InspectorPanel::DrawPortalProperties(PortalNode3d* portal) const {
    const auto& style = EditorStyle::Get();
    Node3d* activeScene = m_editor->GetState() == Editor::State::Playing ? m_editor->GetCore().GetScene() : m_editor->GetActiveViewport()->GetScene();

    SectionHeader("Portal Link");

    std::vector<PortalNode3d*> candidates;
    CollectPortals(activeScene, portal, candidates);

    const PortalNode3d* linked = portal->GetLinkedPortal();
    if (linked) {
        style.PushSuccessText();
        ImGui::Text("Linked: %s", linked->GetName().c_str());
        EditorStyle::PopSuccessText();
    } else {
        style.PushLabel();
        ImGui::TextUnformatted("Not linked");
        EditorStyle::PopLabel();
    }

    ImGui::Spacing();

    if (candidates.empty()) {
        style.PushLabel();
        ImGui::TextUnformatted("No other portals in scene.");
        EditorStyle::PopLabel();
        return;
    }

    BeginPropertyTable();
    PropertyLabel("Target");

    const char* preview = linked ? linked->GetName().c_str() : "None";
    if (ImGui::BeginCombo("##portal_target", preview)) {
        if (ImGui::Selectable("None", linked == nullptr)) {
            portal->Unlink();
        }

        for (PortalNode3d* c : candidates) {
            const bool selected = (linked == c);
            if (ImGui::Selectable(c->GetName().c_str(), selected)) {
                portal->LinkTo(c);
            }

            if (selected) ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }

    if (linked) {
        ImGui::SameLine(0, 6);
        if (ImGui::SmallButton("×")) {
            portal->Unlink();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Unlink");
        }
    }

    EndPropertyTable();
}

void InspectorPanel::DrawPropertyValue(const std::string& name, PropertyHolder* holder) {
    const auto& style = EditorStyle::Get();
    PropertyValue value = holder->GetProperty(name);

    ImGui::PushID(name.c_str());

    std::visit([&]<typename T0>(T0&& val) {
        using T = std::decay_t<T0>;

        if constexpr (std::is_same_v<T, float>) {
            PropertyLabel(name.c_str());
            float v = val;
            if (ImGui::DragFloat("##v", &v, 0.1f)) {
                holder->Set(name, v);
            }
        }

        if constexpr (std::is_same_v<T, int>) {
            PropertyLabel(name.c_str());
            int v = val;
            if (ImGui::DragInt("##v", &v, 1)) {
                holder->Set(name, v);
            }
        }

        if constexpr (std::is_same_v<T, bool>) {
            PropertyLabel(name.c_str());
            bool v = val;
            if (ImGui::Checkbox("##v", &v)) {
                holder->Set(name, v);
            }
        }

        if constexpr (std::is_same_v<T, std::string>) {
            if (name == "mesh_type") {
                static constexpr const char* meshTypes[] = {
                    "None","Cube","Sphere","Plane","Quad","Cylinder","Capsule"
                };

                PropertyLabel("Mesh Type");

                int idx = 0;
                for (int i = 0; i < IM_ARRAYSIZE(meshTypes); i++) {
                    if (val == meshTypes[i]) { idx = i; break; }
                }

                if (ImGui::Combo("##v", &idx, meshTypes, IM_ARRAYSIZE(meshTypes))) {
                    holder->Set(name, std::string(meshTypes[idx]));
                }

                return;
            }

            if (name == "diffuse" || name == "normal" || name == "metallic" || name == "roughness" || name == "ambient_occlusion" || name == "emissive" || name == "displacement") {
                ImGui::Spacing();

                if (val.empty()) {
                    style.PushLabel();
                    ImGui::TextUnformatted(name.c_str());
                    ImGui::SameLine();
                    ImGui::TextDisabled("None");
                    EditorStyle::PopLabel();
                    ImGui::Spacing();
                    return;
                }

                // get the texture from resource manager for display
                const auto tex = GetCachedTexture(val);
                if (!tex) {
                    style.PushLabel();
                    ImGui::TextUnformatted(name.c_str());
                    ImGui::SameLine();
                    ImGui::TextDisabled("Failed to load");
                    EditorStyle::PopLabel();
                    ImGui::Spacing();
                    return;
                }

                const bool clicked = ImGui::ImageButton("##thumb", static_cast<ImTextureID>(static_cast<intptr_t>(tex->GetGLHandle())), ImVec2(40, 40));
                if (clicked) {
                    OpenTexturePreview(tex, name.c_str());
                }

                ImGui::SameLine();
                ImGui::BeginGroup();

                style.PushAccentText();
                ImGui::TextUnformatted(name.c_str());
                EditorStyle::PopAccentText();

                style.PushLabel();
                ImGui::Text("%dx%d  •  %dch", tex->GetWidth(), tex->GetHeight(), tex->GetChannels());
                EditorStyle::PopLabel();

                ImGui::EndGroup();

                const std::string treeId = name + "_props";
                style.PushLabel();
                const bool open = ImGui::TreeNodeEx(treeId.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth, "Details");
                EditorStyle::PopLabel();

                if (open) {
                    ImGui::PushStyleColor(ImGuiCol_TableBorderLight, style.colSeparator);
                    DrawProperties(tex.get());
                    ImGui::PopStyleColor();
                    ImGui::TreePop();
                }

                ImGui::Spacing();
                return;
            }

            PropertyLabel(name.c_str());

            char buf[256];
            strncpy(buf, val.c_str(), sizeof(buf));
            buf[sizeof(buf) - 1] = '\0';

            if (ImGui::InputText("##v", buf, sizeof(buf))) {
                holder->Set(name, std::string(buf));
            }
        }

        if constexpr (std::is_same_v<T, glm::vec2>) {
            PropertyLabel(name.c_str());
            glm::vec2 v = val;
            if (ImGui::DragFloat2("##v", &v.x, 0.1f)) {
                holder->Set(name, v);
            }
        }

        if constexpr (std::is_same_v<T, glm::vec3>) {
            PropertyLabel(name.c_str());
            glm::vec3 v = val;

            if (name == "albedo_color" || name == "emission_color" || name == "color") {
                if (ImGui::ColorEdit3("##v", &v.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_PickerHueWheel)) {
                    holder->Set(name, v);
                }
            } else {
                if (ImGui::DragFloat3("##v", &v.x, 0.1f)) {
                    holder->Set(name, v);
                }
            }
        }

        if constexpr (std::is_same_v<T, std::shared_ptr<PropertyHolder>>) {
            if (!val) return;

            ImGui::Spacing();

            std::string label = name;
            if (!label.empty()) {
                label[0] = static_cast<char>(toupper(label[0]));
            }

            style.PushAccentText();
            const bool open = ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding, "%s", label.c_str());
            EditorStyle::PopAccentText();

            if (open) {
                ImGui::PushStyleColor(ImGuiCol_TableBorderLight, style.colSeparator);
                DrawProperties(val.get());
                ImGui::PopStyleColor();
                ImGui::TreePop();
            }

            ImGui::Spacing();
        }
    }, value);

    ImGui::PopID();
}

void InspectorPanel::DrawTexturePreviewPopup() {
    const auto& style = EditorStyle::Get();

    if (!m_previewTex) return;

    ImGui::SetNextWindowSize(ImVec2(520, 580), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    bool open = true;
    if (ImGui::Begin(m_previewLabel.c_str(), &open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize)) {
        const float panelW = ImGui::GetContentRegionAvail().x;

        static int channel = 0;
        static bool flipY = false;
        static float zoom = 1.0f;

        // controls row
        ImGui::SetNextItemWidth(150);
        ImGui::Combo("Channel##prev", &channel, "RGB\0Red\0Green\0Blue\0Alpha\0");
        ImGui::SameLine(0, 12);
        ImGui::Checkbox("Flip Y", &flipY);
        ImGui::SameLine(0, 12);
        ImGui::SetNextItemWidth(110);
        ImGui::SliderFloat("Zoom", &zoom, 0.25f, 4.0f, "%.2fx");
        ImGui::SameLine(0, 8);
        if (ImGui::SmallButton("Reset")) {
            zoom = 1.0f;
            channel = 0;
            flipY = false;
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        const float imgSize = panelW * zoom;
        const ImVec2 uv0 = flipY ? ImVec2(0, 1) : ImVec2(0, 0);
        const ImVec2 uv1 = flipY ? ImVec2(1, 0) : ImVec2(1, 1);

        ImVec4 tint = {1, 1, 1, 1};
        if (channel == 1) tint = {1, 0, 0, 1};
        else if (channel == 2) tint = {0, 1, 0, 1};
        else if (channel == 3) tint = {0, 0, 1, 1};

        ImGui::BeginChild("##scroll", ImVec2(panelW, panelW), false, ImGuiWindowFlags_HorizontalScrollbar);

        const ImVec2 imgPos = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        constexpr float cs = 12.0f;

        for (float cy = 0; cy < imgSize; cy += cs) {
            for (float cx = 0; cx < imgSize; cx += cs) {
                const bool even = ((int)(cx / cs) + (int)(cy / cs)) % 2 == 0;
                dl->AddRectFilled(
                    { imgPos.x + cx,      imgPos.y + cy },
                    { imgPos.x + cx + cs, imgPos.y + cy + cs },
                    even ? style.colCheckerLightU32 : style.colCheckerDarkU32
                );
            }
        }

        ImGui::Image(
            static_cast<ImTextureID>(static_cast<intptr_t>(m_previewTex->GetGLHandle())),
            { imgSize, imgSize }, uv0, uv1, tint, {0, 0, 0, 0}
        );

        ImGui::EndChild();

        ImGui::Separator();
        ImGui::Spacing();

        style.PushLabel();
        ImGui::Text("%d x %d  |  %d channels  |  GL #%u",
            m_previewTex->GetWidth(),
            m_previewTex->GetHeight(),
            m_previewTex->GetChannels(),
            m_previewTex->GetGLHandle()
        );
        EditorStyle::PopLabel();
    }
    ImGui::End();

    if (!open) m_previewTex = nullptr;
}

std::shared_ptr<Texture> InspectorPanel::GetCachedTexture(const std::string &path) {
    if (path.empty()) return nullptr;

    if (const auto it = m_textureCache.find(path); it != m_textureCache.end()) {
        return it->second;
    }

    auto tex = ResourceManager::Get().LoadTexture(path);
    if (tex) {
        m_textureCache[path] = tex;
    }

    return tex;
}

void InspectorPanel::OpenTexturePreview(const std::shared_ptr<Texture>& tex, const char* label) {
    m_previewTex = tex;
    m_previewLabel = std::string("Texture: ") + label + "###TexPreview";
}
