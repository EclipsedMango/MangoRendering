
#include "Editor.h"

#include <functional>

#include "Input.h"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.inl"
#include "Nodes/MeshNode3d.h"
#include "Nodes/CameraNode3d.h"
#include "Nodes/Lights/DirectionalLightNode3d.h"
#include "Nodes/Lights/PointLightNode3d.h"
#include "Renderer/Meshes/PrimitiveMesh.h"

Editor::Editor(Node3d* scene) : m_core(scene) {
    m_core.Init();

    const glm::vec2 winSize = m_core.GetActiveWindow()->GetSize();
    const float aspect = winSize.y != 0.0f ? winSize.x / winSize.y : 1.0f;

    m_editorCamera = std::make_unique<CameraNode3d>(glm::vec3(0, 2, 6), 75.0f, aspect);

    m_core.SetEditorCamera(m_editorCamera.get());
    m_core.SetCameraMode(Core::CameraMode::Editor);

    SDL_SetWindowRelativeMouseMode(m_core.GetActiveWindow()->GetSDLWindow(), false);
    Input::SetMouseDeltaEnabled(false);
}

Editor::~Editor() {
    SDL_SetWindowRelativeMouseMode(m_core.GetActiveWindow()->GetSDLWindow(), false);
    Input::SetMouseDeltaEnabled(false);
}

void Editor::Run() {
    uint64_t lastTime = SDL_GetTicksNS();

    while (m_core.GetActiveWindow()->IsOpen()) {
        const uint64_t now = SDL_GetTicksNS();
        const float deltaTime = (now - lastTime) / 1e9f;
        lastTime = now;

        m_core.PollEvents();

        if (Input::IsKeyJustPressed(SDL_SCANCODE_DELETE)) {
            DeleteSelectedNodes();
        }

        Core::BeginImGuiFrame();
        DrawGizmo();

        if (m_state == State::Playing) {
            m_core.StepFrame(deltaTime);
        } else {
            UpdateEditorCamera(deltaTime);
            m_core.GetScene()->UpdateWorldTransform();
        }

        DrawMenuBar();
        DrawSceneTree(m_core.GetScene());
        DrawInspector();
        DrawContentBrowser();

        m_core.RenderScene();
        Core::EndImGuiFrame();
        m_core.SwapBuffers();
    }
}

void Editor::UpdateEditorCamera(const float dt) {
    if (!m_editorCamera) return;

    const ImGuiIO& io = ImGui::GetIO();
    const bool imguiWantsMouse = io.WantCaptureMouse;
    const bool imguiWantsKeys = io.WantCaptureKeyboard;

    const bool rmbHeld = Input::IsMouseButtonHeld(SDL_BUTTON_RIGHT);
    const bool shouldLook = rmbHeld && !imguiWantsMouse;

    if (!m_rmbLook) {
        if (Input::IsKeyJustPressed(SDL_SCANCODE_W)) m_gizmoOp = ImGuizmo::TRANSLATE;
        if (Input::IsKeyJustPressed(SDL_SCANCODE_E)) m_gizmoOp = ImGuizmo::ROTATE;
        if (Input::IsKeyJustPressed(SDL_SCANCODE_R)) m_gizmoOp = ImGuizmo::SCALE;
    }

    if (shouldLook && !m_rmbLook) {
        m_rmbLook = true;
        SDL_SetWindowRelativeMouseMode(m_core.GetActiveWindow()->GetSDLWindow(), true);
        Input::SetMouseDeltaEnabled(true);
    } else if (!shouldLook && m_rmbLook) {
        m_rmbLook = false;
        SDL_SetWindowRelativeMouseMode(m_core.GetActiveWindow()->GetSDLWindow(), false);
        Input::SetMouseDeltaEnabled(false);
    }

    if (m_rmbLook && !imguiWantsMouse) {
        const float wheelY = Input::GetMouseWheelY();
        if (wheelY != 0.0f) {
            m_moveSpeed *= std::pow(1.1f, wheelY);
            m_moveSpeed = glm::clamp(m_moveSpeed, 0.05f, 500.0f);
        }
    }

    if (!m_rmbLook) return;

    const glm::vec2 md = Input::GetMouseDelta();
    m_editorCamera->Rotate(md.x * m_mouseSensitivity, -md.y * m_mouseSensitivity);

    if (!imguiWantsKeys) {
        float speed = m_moveSpeed;
        if (Input::IsKeyHeld(SDL_SCANCODE_LSHIFT)) speed *= 5.0f;

        glm::vec3 dir(0.0f);
        if (Input::IsKeyHeld(SDL_SCANCODE_W)) dir += m_editorCamera->GetFront();
        if (Input::IsKeyHeld(SDL_SCANCODE_S)) dir -= m_editorCamera->GetFront();
        if (Input::IsKeyHeld(SDL_SCANCODE_D)) dir += m_editorCamera->GetRight();
        if (Input::IsKeyHeld(SDL_SCANCODE_A)) dir -= m_editorCamera->GetRight();
        if (Input::IsKeyHeld(SDL_SCANCODE_E)) dir += glm::vec3(0, 1, 0);
        if (Input::IsKeyHeld(SDL_SCANCODE_Q)) dir -= glm::vec3(0, 1, 0);

        if (glm::length(dir) > 0.0f) {
            dir = glm::normalize(dir);
            m_editorCamera->SetPosition(m_editorCamera->GetPosition() + dir * speed * dt);
        }
    }
}

void Editor::DrawMenuBar() {
    if (!ImGui::BeginMainMenuBar()) return;

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New Scene"))  { /* TODO */ }
        if (ImGui::MenuItem("Open Scene")) { /* TODO */ }
        if (ImGui::MenuItem("Save Scene")) { /* TODO */ }
        ImGui::Separator();
        if (ImGui::MenuItem("Exit")) m_core.GetActiveWindow()->Close();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
        if (ImGui::MenuItem("Undo")) { /* TODO */ }
        if (ImGui::MenuItem("Redo")) { /* TODO */ }
        ImGui::EndMenu();
    }

    const float buttonWidth = 60.0f;
    const float spacing     = 4.0f;
    const float totalWidth  = buttonWidth * 3 + spacing * 2;
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f + ImGui::GetCursorPosX());

    const bool isPlaying = m_state == State::Playing;
    const bool isPaused  = m_state == State::Paused;
    const bool isEditing = m_state == State::Editing;

    // play
    if (isPlaying) ImGui::BeginDisabled();
    if (ImGui::Button("Play", {buttonWidth, 0})) OnPlay();
    if (isPlaying) ImGui::EndDisabled();

    ImGui::SameLine(0, spacing);

    // pause
    if (isEditing) ImGui::BeginDisabled();
    if (ImGui::Button("Pause", {buttonWidth, 0})) OnPause();
    if (isEditing) ImGui::EndDisabled();

    ImGui::SameLine(0, spacing);

    // stop
    if (isEditing) ImGui::BeginDisabled();
    if (ImGui::Button("Stop", {buttonWidth, 0})) OnStop();
    if (isEditing) ImGui::EndDisabled();

    ImGui::EndMainMenuBar();
}

void Editor::DrawSceneTree(Node3d* node) {
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
                Node3d* root = m_core.GetScene();
                Node3d* created = nullptr;

                switch (i) {
                    case 0: {
                        created = new Node3d();
                        created->SetName("Node3d");
                        break;
                    }
                    case 1: {
                        created = new MeshNode3d(std::make_shared<CubeMesh>(), m_core.GetDefaultShader());
                        created->SetName("MeshNode3d");
                        break;
                    }
                    case 2: {
                        const glm::vec2 ws = m_core.GetActiveWindow()->GetSize();
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

    constexpr ImGuiMultiSelectFlags msFlags =
        ImGuiMultiSelectFlags_ClearOnEscape |
        ImGuiMultiSelectFlags_BoxSelect1d;

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

void Editor::DrawInspector() {
    ImGui::Begin("Inspector");

    if (!m_lastSelectedNode) {
        ImGui::TextDisabled("No node selected");
        ImGui::End();
        return;
    }

    char nameBuf[256];
    strncpy(nameBuf, m_lastSelectedNode->GetName().c_str(), sizeof(nameBuf));
    nameBuf[sizeof(nameBuf) - 1] = '\0';
    if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) m_lastSelectedNode->SetName(nameBuf);

    ImGui::Separator();

    // transform
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        glm::vec3 pos = m_lastSelectedNode->GetPosition();
        glm::vec3 rot = m_lastSelectedNode->GetRotationEuler();
        glm::vec3 scl = m_lastSelectedNode->GetScale();

        if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) m_lastSelectedNode->SetPosition(pos);
        if (ImGui::DragFloat3("Rotation", &rot.x, 0.5f)) m_lastSelectedNode->SetRotationEuler(rot);
        if (ImGui::DragFloat3("Scale", &scl.x, 0.1f)) {
            scl.x = std::abs(scl.x) < 1e-4f ? 1e-4f : scl.x;
            scl.y = std::abs(scl.y) < 1e-4f ? 1e-4f : scl.y;
            scl.z = std::abs(scl.z) < 1e-4f ? 1e-4f : scl.z;
            m_lastSelectedNode->SetScale(scl);
        }
    }

    // mesh node
    if (auto* mesh = dynamic_cast<MeshNode3d*>(m_lastSelectedNode)) {
        if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen)) {
            const char* primitiveNames[] = { "Cube", "Plane", "Quad", "Sphere", "Cylinder", "Capsule" };

            // figure out which item is currently selected
            auto meshPtr = mesh->GetMeshPtr();
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
                        case 0: mesh->SetMesh(std::make_shared<CubeMesh>()); break;
                        case 1: mesh->SetMesh(std::make_shared<PlaneMesh>()); break;
                        case 2: mesh->SetMesh(std::make_shared<QuadMesh>()); break;
                        case 3: mesh->SetMesh(std::make_shared<SphereMesh>()); break;
                        case 4: mesh->SetMesh(std::make_shared<CylinderMesh>()); break;
                        case 5: mesh->SetMesh(std::make_shared<CapsuleMesh>()); break;
                        default: mesh->SetMesh(std::make_shared<CubeMesh>()); break;
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

        DrawMaterialInspector(mesh->GetActiveMaterial());
        DrawTexturePreviewPopup();
    }

    // point light node
    if (auto* pl = dynamic_cast<PointLightNode3d*>(m_lastSelectedNode)) {
        if (ImGui::CollapsingHeader("Point Light", ImGuiTreeNodeFlags_DefaultOpen)) {
            glm::vec4 color = glm::vec4(pl->GetColor(), 1.0f);
            if (ImGui::ColorEdit4("Color", &color.x)) pl->SetColor(color);

            float rad = pl->GetLight()->GetRadius();
            if (ImGui::DragFloat("Radius", &rad, 0.1f)) pl->SetRadius(rad);

            float intensity = pl->GetIntensity();
            if (ImGui::DragFloat("Intensity", &intensity, 0.1f)) pl->SetIntensity(intensity);

            if (ImGui::CollapsingHeader("Attenuation", ImGuiTreeNodeFlags_DefaultOpen)) {
                float constant = pl->GetLight()->GetConstant();
                float linear = pl->GetLight()->GetLinear();
                float quad = pl->GetLight()->GetQuadratic();
                if (ImGui::SliderFloat("Constant", &constant, 0.0f, 1.0f)) pl->GetLight()->SetAttenuation(constant, linear, quad);
                if (ImGui::SliderFloat("Linear", &linear, 0.0f, 1.0f)) pl->GetLight()->SetAttenuation(constant, linear, quad);
                if (ImGui::SliderFloat("Quad", &quad, 0.0f, 1.0f)) pl->GetLight()->SetAttenuation(constant, linear, quad);
            }
        }
    }

    // directional light node
    if (auto* dl = dynamic_cast<DirectionalLightNode3d*>(m_lastSelectedNode)) {
        if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
            glm::vec4 color = glm::vec4(dl->GetColor(), 1.0f);
            if (ImGui::ColorEdit4("Color", &color.x)) dl->SetColor(color);

            float intensity = dl->GetIntensity();
            if (ImGui::DragFloat("Intensity", &intensity, 0.1f)) dl->SetIntensity(intensity);
        }
    }

    ImGui::End();
}

static void TextureSlot(const char* label, const std::shared_ptr<Texture>& tex, Editor* editor) {
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
            editor->OpenTexturePreview(tex.get(), label);
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

static void SectionHeader(const char* label) {
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(180, 180, 180, 255));
    ImGui::SeparatorText(label);
    ImGui::PopStyleColor();
}

void Editor::DrawMaterialInspector(Material &mat) {
    if (!ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    ImGui::PushID("MatInspector");

    // ----- albedo ----
    SectionHeader("Albedo");
    glm::vec4 color = mat.GetAlbedoColor();
    if (ImGui::ColorEdit4("Color##albedo", &color.x, ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_AlphaBar)) {
        mat.SetAlbedoColor(color);
    }

    if (ImGui::BeginTable("##albedoTex", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerH)) {
        TextureSlot("Texture (Diffuse)", mat.GetDiffuse(), this);
        ImGui::EndTable();
    }

    // ----- metallic / roughness -----
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
        TextureSlot("Metallic/Roughness Map", mat.GetMetallic(), this);
        ImGui::EndTable();
    }

    // show packed indicator
    if (mat.GetMetallic() && mat.GetRoughness() && mat.GetMetallic() == mat.GetRoughness()) {
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(120, 200, 255, 255));
        ImGui::TextUnformatted("  (Packed: G=Roughness, B=Metallic)");
        ImGui::PopStyleColor();
    }

    // ----- normal map -----
    SectionHeader("Normal Map");
    if (ImGui::BeginTable("##normalTex", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerH)) {
        TextureSlot("Normal Map", mat.GetNormal(), this);
        ImGui::EndTable();
    }

    float normalStr = mat.GetNormalStrength();
    ImGui::PushItemWidth(-1);
    ImGui::Text("Strength");
    if (ImGui::SliderFloat("##normalStr", &normalStr, 0.0f, 4.0f, "%.3f")) {
        mat.SetNormalStrength(normalStr);
    }

    ImGui::PopItemWidth();

    // ----- ambient occlusion -----
    SectionHeader("Ambient Occlusion");
    if (ImGui::BeginTable("##aoTex", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerH)) {
        TextureSlot("AO Map", mat.GetAmbientOcclusion(), this);
        ImGui::EndTable();
    }

    float aoStr = mat.GetAOStrength();
    ImGui::PushItemWidth(-1);
    ImGui::Text("Strength");
    if (ImGui::SliderFloat("##aoStr", &aoStr, 0.0f, 1.0f, "%.3f")) {
        mat.SetAOStrength(aoStr);
    }

    ImGui::PopItemWidth();

    // ----- emission -----
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
        TextureSlot("Emissive Map", mat.GetEmissive(), this);
        ImGui::EndTable();
    }

    // ----- uv -----
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

    // ----- render flags -----
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
    const char* blendModes[] = { "Opaque", "Alpha Blend", "Alpha Scissor" };
    int blendIndex = 0;
    switch (mat.GetBlendMode()) {
        case BlendMode::Opaque: blendIndex = 0; break;
        case BlendMode::AlphaBlend: blendIndex = 1; break;
        case BlendMode::AlphaScissor: blendIndex = 2; break;
        case BlendMode::Additive: break;
    }

    ImGui::Text("Blend Mode");
    if (ImGui::Combo("##blendMode", &blendIndex, blendModes, IM_ARRAYSIZE(blendModes))) {
        switch (blendIndex) {
            case 0: mat.SetBlendMode(BlendMode::Opaque); break;
            case 1: mat.SetBlendMode(BlendMode::AlphaBlend); break;
            case 2: mat.SetBlendMode(BlendMode::AlphaScissor); break;
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


void Editor::DrawContentBrowser() {
    ImGui::Begin("Content Browser");
    ImGui::TextDisabled("Content browser coming soon");
    ImGui::End();
}

void Editor::DrawGizmo() {
    ImGuizmo::BeginFrame();

    if (!m_lastSelectedNode || m_state == State::Playing || !m_editorCamera) return;

    const glm::vec2 winSize = m_core.GetActiveWindow()->GetSize();
    ImGuizmo::SetRect(0, 0, winSize.x, winSize.y);
    ImGuizmo::Enable(!m_rmbLook);

    glm::mat4 view  = m_editorCamera->GetViewMatrix();
    glm::mat4 proj  = m_editorCamera->GetProjectionMatrix();
    glm::mat4 world = m_lastSelectedNode->GetWorldMatrix();
    glm::mat4 delta = glm::mat4(1.0f);

    ImGuizmo::Manipulate(
        glm::value_ptr(view),
        glm::value_ptr(proj),
        m_gizmoOp,
        m_gizmoMode,
        glm::value_ptr(world),
        glm::value_ptr(delta)
    );

    if (!ImGuizmo::IsUsing()) return;

    glm::mat4 localMatrix = world;
    if (const Node3d* parent = m_lastSelectedNode->GetParent())
        localMatrix = glm::inverse(parent->GetWorldMatrix()) * world;

    const glm::vec3 scale = {
        glm::length(glm::vec3(localMatrix[0])),
        glm::length(glm::vec3(localMatrix[1])),
        glm::length(glm::vec3(localMatrix[2]))
    };

    constexpr float eps = 1e-6f;
    glm::mat3 rotMat = {
        scale.x > eps ? glm::vec3(localMatrix[0]) / scale.x : glm::vec3(1,0,0),
        scale.y > eps ? glm::vec3(localMatrix[1]) / scale.y : glm::vec3(0,1,0),
        scale.z > eps ? glm::vec3(localMatrix[2]) / scale.z : glm::vec3(0,0,1)
    };

    const float det = glm::determinant(rotMat);
    if (std::abs(det - 1.0f) < 0.01f) {
        m_lastSelectedNode->SetRotation(glm::quat_cast(rotMat));
    } else {
        rotMat[0] = glm::normalize(rotMat[0]);
        rotMat[1] = glm::normalize(glm::cross(glm::vec3(rotMat[2]), glm::vec3(rotMat[0])));
        rotMat[2] = glm::normalize(glm::cross(glm::vec3(rotMat[0]), glm::vec3(rotMat[1])));
        m_lastSelectedNode->SetRotation(glm::quat_cast(rotMat));
    }

    m_lastSelectedNode->SetPosition(glm::vec3(localMatrix[3]));
    m_lastSelectedNode->SetScale(scale);
}

void Editor::DrawTexturePreviewPopup() {
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

void Editor::OpenTexturePreview(const Texture *tex, const char *label) {
    m_previewTex = tex;
    m_previewLabel = std::string("Texture: ") + label + "###TexPreview";
}

void Editor::OnPlay() {
    // TODO: serialize scene state here so we can restore on stop
    m_state = State::Playing;

    m_core.SetCameraMode(Core::CameraMode::Game);

    m_rmbLook = false;
    SDL_SetWindowRelativeMouseMode(m_core.GetActiveWindow()->GetSDLWindow(), false);
    Input::SetMouseDeltaEnabled(false);
}

void Editor::OnPause() {
    if (m_state == State::Playing) {
        m_state = State::Paused;
        m_core.SetCameraMode(Core::CameraMode::Editor);
    } else if (m_state == State::Paused) {
        m_state = State::Playing;
        m_core.SetCameraMode(Core::CameraMode::Game);
    }
}

void Editor::OnStop() {
    // TODO: restore serialized scene state here
    m_state = State::Editing;

    m_core.SetCameraMode(Core::CameraMode::Editor);

    m_rmbLook = false;
    SDL_SetWindowRelativeMouseMode(m_core.GetActiveWindow()->GetSDLWindow(), false);
    Input::SetMouseDeltaEnabled(false);
}

void Editor::DeleteSelectedNodes() {
    if (!m_lastSelectedNode || m_lastSelectedNode == m_core.GetScene()) {
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
        Node3d* n = FindNodeById(m_core.GetScene(), id);
        if (!n || n == m_core.GetScene()) {
            continue;
        }

        if (n == m_lastSelectedNode) {
            m_lastSelectedNode = nullptr;
        }

        // n->QueueFree();

        if (Node3d* p = n->GetParent()) p->RemoveChild(n);
        delete n;
    }

    m_selection.Clear();
}

Node3d* Editor::FindNodeById(Node3d *root, uint32_t id) {
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

int Editor::CountNodesRecursive(const Node3d *root) {
    if (!root) {
        return 0;
    }

    int count = 1;
    for (Node3d* c : root->GetChildren()) {
        count += CountNodesRecursive(c);
    }

    return count;
}
