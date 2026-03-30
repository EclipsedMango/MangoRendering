
#include "InspectorPanel.h"

#include <iostream>
#include <memory>

#include "../Editor.h"
#include "imgui.h"
#include "Core/ResourceManager.h"
#include "glm/glm.hpp"
#include "Nodes/Node3d.h"
#include "Nodes/PortalNode3d.h"

static constexpr ImVec4 COL_HEADER = {0.55f, 0.75f, 0.95f, 1.00f}; // soft blue
static constexpr ImVec4 COL_LABEL = {0.75f, 0.75f, 0.75f, 1.00f}; // muted grey
static constexpr ImVec4 COL_SEPARATOR = {0.30f, 0.30f, 0.35f, 1.00f};
static constexpr ImU32  COL_TREE_LINE = IM_COL32(80, 80, 90, 200);

InspectorPanel::InspectorPanel(Editor* editor) : m_editor(editor) {}

static void SectionHeader(const char* label) {
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, COL_HEADER);
    ImGui::SeparatorText(label);
    ImGui::PopStyleColor();
    ImGui::Spacing();
}

static void BeginPropertyTable() {
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4, 3));
    ImGui::BeginTable("##props", 2,
        ImGuiTableFlags_SizingStretchProp |
        ImGuiTableFlags_BordersInnerV,
        ImVec2(0, 0)
    );
    ImGui::TableSetupColumn("##label", ImGuiTableColumnFlags_WidthFixed, 130.0f);
    ImGui::TableSetupColumn("##value", ImGuiTableColumnFlags_WidthStretch);
}

static void EndPropertyTable() {
    ImGui::EndTable();
    ImGui::PopStyleVar();
}

static void PropertyLabel(const char* label) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::PushStyleColor(ImGuiCol_Text, COL_LABEL);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-FLT_MIN); // fill remaining width
}

void InspectorPanel::DrawInspector(Node3d* selectedNode) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
    ImGui::Begin("Inspector");

    if (!selectedNode) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, COL_LABEL);
        const float w = ImGui::GetContentRegionAvail().x;
        const char* msg = "No node selected";
        ImGui::SetCursorPosX((w - ImGui::CalcTextSize(msg).x) * 0.5f);
        ImGui::TextUnformatted(msg);
        ImGui::PopStyleColor();
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }

    // Node type badge
    ImGui::PushStyleColor(ImGuiCol_Text, COL_HEADER);
    ImGui::TextUnformatted(selectedNode->GetName().c_str());
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, COL_LABEL);
    ImGui::TextUnformatted("- Node3d");
    ImGui::PopStyleColor();
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

void InspectorPanel::DrawPortalProperties(PortalNode3d* portal) {
    Node3d* activeScene = m_editor->GetState() == Editor::State::Playing ? m_editor->GetCore().GetScene() : m_editor->GetActiveViewport()->GetScene();

    SectionHeader("Portal Link");

    std::vector<PortalNode3d*> candidates;
    CollectPortals(activeScene, portal, candidates);

    const PortalNode3d* linked = portal->GetLinkedPortal();
    if (linked) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.5f, 1.0f));
        ImGui::Text("Linked: %s", linked->GetName().c_str());
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, COL_LABEL);
        ImGui::TextUnformatted("Not linked");
        ImGui::PopStyleColor();
    }

    ImGui::Spacing();

    if (candidates.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, COL_LABEL);
        ImGui::TextUnformatted("No other portals in scene.");
        ImGui::PopStyleColor();
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
                    ImGui::PushStyleColor(ImGuiCol_Text, COL_LABEL);
                    ImGui::TextUnformatted(name.c_str());
                    ImGui::SameLine();
                    ImGui::TextDisabled("None");
                    ImGui::PopStyleColor();
                    ImGui::Spacing();
                    return;
                }

                // get the texture from resource manager for display
                const auto tex = ResourceManager::Get().LoadTexture(val);

                const bool clicked = ImGui::ImageButton(
                    "##thumb",
                    static_cast<ImTextureID>(static_cast<intptr_t>(tex->GetGLHandle())),
                    ImVec2(40, 40)
                );

                if (clicked) OpenTexturePreview(tex.get(), name.c_str());

                ImGui::SameLine();
                ImGui::BeginGroup();
                ImGui::PushStyleColor(ImGuiCol_Text, COL_HEADER);
                ImGui::TextUnformatted(name.c_str());
                ImGui::PopStyleColor();
                ImGui::PushStyleColor(ImGuiCol_Text, COL_LABEL);
                ImGui::Text("%dx%d  •  %dch", tex->GetWidth(), tex->GetHeight(), tex->GetChannels());
                ImGui::PopStyleColor();
                ImGui::EndGroup();

                const std::string treeId = name + "_props";
                ImGui::PushStyleColor(ImGuiCol_Text, COL_LABEL);
                const bool open = ImGui::TreeNodeEx(treeId.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth, "Details");
                ImGui::PopStyleColor();
                if (open) {
                    DrawProperties(tex.get());
                    ImGui::TreePop();
                }

                ImGui::Spacing();
                return;
            }

            PropertyLabel(name.c_str());
            char buf[256];
            strncpy(buf, val.c_str(), sizeof(buf));
            buf[sizeof(buf)-1] = '\0';
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
            ImGui::PushStyleColor(ImGuiCol_Text, COL_HEADER);

            // capitalise first letter for display
            std::string label = name;
            if (!label.empty()) label[0] = static_cast<char>(toupper(label[0]));

            const bool open = ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding, "%s", label.c_str());
            ImGui::PopStyleColor();

            if (open) {
                ImGui::PushStyleColor(ImGuiCol_TableBorderLight, COL_SEPARATOR);
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
        if (ImGui::SmallButton("Reset")) { zoom = 1.0f; channel = 0; flipY = false; }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        const float imgSize = panelW * zoom;
        const ImVec2 uv0 = flipY ? ImVec2(0,1) : ImVec2(0,0);
        const ImVec2 uv1 = flipY ? ImVec2(1,0) : ImVec2(1,1);

        ImVec4 tint = {1,1,1,1};
        if (channel == 1) tint = {1,0,0,1};
        else if (channel == 2) tint = {0,1,0,1};
        else if (channel == 3) tint = {0,0,1,1};

        ImGui::BeginChild("##scroll", ImVec2(panelW, panelW), false, ImGuiWindowFlags_HorizontalScrollbar);

        // checkerboard background
        const ImVec2 imgPos = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        constexpr float cs = 12.0f;
        for (float cy = 0; cy < imgSize; cy += cs) {
            for (float cx = 0; cx < imgSize; cx += cs) {
                const bool even = ((int)(cx/cs)+(int)(cy/cs)) % 2 == 0;
                dl->AddRectFilled(
                    {imgPos.x+cx,    imgPos.y+cy},
                    {imgPos.x+cx+cs, imgPos.y+cy+cs},
                    even ? IM_COL32(80,80,80,255) : IM_COL32(50,50,50,255)
                );
            }
        }

        ImGui::Image(
            static_cast<ImTextureID>(static_cast<intptr_t>(m_previewTex->GetGLHandle())),
            {imgSize, imgSize}, uv0, uv1, tint, {0,0,0,0}
        );

        ImGui::EndChild();

        ImGui::Separator();
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, COL_LABEL);
        ImGui::Text("%d x %d  |  %d channels  |  GL #%u",
            m_previewTex->GetWidth(), m_previewTex->GetHeight(),
            m_previewTex->GetChannels(), m_previewTex->GetGLHandle()
        );
        ImGui::PopStyleColor();
    }
    ImGui::End();

    if (!open) m_previewTex = nullptr;
}

void InspectorPanel::OpenTexturePreview(const Texture *tex, const char *label) {
    m_previewTex = tex;
    m_previewLabel = std::string("Texture: ") + label + "###TexPreview";
}