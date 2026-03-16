
#include "Editor.h"

#include <functional>

#include "Input.h"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.inl"
#include "Nodes/MeshNode3d.h"
#include "Nodes/CameraNode3d.h"
#include "Nodes/Lights/DirectionalLightNode3d.h"
#include "Nodes/Lights/PointLightNode3d.h"
#include "Nodes/Lights/SpotLightNode3d.h"

Editor::Editor(Node3d* scene) : m_core(scene) {
    m_core.Init();

    const glm::vec2 winSize = m_core.GetActiveWindow()->GetSize();
    const float aspect = (winSize.y != 0.0f) ? (winSize.x / winSize.y) : 1.0f;

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
        Core::BeginImGuiFrame();

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

        DrawGizmo();
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
    std::function<void(Node3d*)> drawNode = [&](Node3d* n) {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (n == m_selectedNode)      flags |= ImGuiTreeNodeFlags_Selected;
        if (n->GetChildren().empty()) flags |= ImGuiTreeNodeFlags_Leaf;

        // use pointer as unique id, label as node type
        const char* label = "Node3d";
        if      (dynamic_cast<MeshNode3d*>(n))              label = "MeshNode3d";
        else if (dynamic_cast<DirectionalLightNode3d*>(n))  label = "DirectionalLight";
        else if (dynamic_cast<PointLightNode3d*>(n))        label = "PointLight";
        else if (dynamic_cast<SpotLightNode3d*>(n))         label = "SpotLight";
        else if (dynamic_cast<CameraNode3d*>(n))            label = "Camera";

        const bool open = ImGui::TreeNodeEx((void*)n, flags, "%s", label);
        if (ImGui::IsItemClicked()) m_selectedNode = n;

        if (open) {
            for (auto* child : n->GetChildren()) drawNode(child);
            ImGui::TreePop();
        }
    };

    drawNode(node);
    ImGui::End();
}

void Editor::DrawInspector() const {
    ImGui::Begin("Inspector");

    if (!m_selectedNode) {
        ImGui::TextDisabled("No node selected");
        ImGui::End();
        return;
    }

    // transform
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        glm::vec3 pos = m_selectedNode->GetPosition();
        glm::vec3 rot = m_selectedNode->GetRotationEuler();
        glm::vec3 scl = m_selectedNode->GetScale();

        if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) m_selectedNode->SetPosition(pos);
        if (ImGui::DragFloat3("Rotation", &rot.x, 0.5f)) m_selectedNode->SetRotationEuler(rot);
        if (ImGui::DragFloat3("Scale",    &scl.x, 0.1f)) m_selectedNode->SetScale(scl);
    }

    // mesh node
    if (auto* mesh = dynamic_cast<MeshNode3d*>(m_selectedNode)) {
        if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& mat = mesh->GetActiveMaterial();

            glm::vec4 color = mat.GetAlbedoColor();
            if (ImGui::ColorEdit4("Albedo", &color.x)) mat.SetAlbedoColor(color);

            float metallic  = mat.GetMetallicValue();
            float roughness = mat.GetRoughnessValue();
            if (ImGui::SliderFloat("Metallic",  &metallic,  0.0f, 1.0f)) mat.SetMetallicValue(metallic);
            if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f)) mat.SetRoughnessValue(roughness);

            bool doubleSided = mat.GetDoubleSided();
            if (ImGui::Checkbox("Double Sided", &doubleSided)) mat.SetDoubleSided(doubleSided);

            bool castShadows = mat.GetCastShadows();
            if (ImGui::Checkbox("Cast Shadows", &castShadows)) mat.SetCastShadows(castShadows);
        }
    }

    // point light node
    if (auto* pl = dynamic_cast<PointLightNode3d*>(m_selectedNode)) {
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
    if (auto* dl = dynamic_cast<DirectionalLightNode3d*>(m_selectedNode)) {
        if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
            glm::vec4 color = glm::vec4(dl->GetColor(), 1.0f);
            if (ImGui::ColorEdit4("Color", &color.x)) dl->SetColor(color);

            float intensity = dl->GetIntensity();
            if (ImGui::DragFloat("Intensity", &intensity, 0.1f)) dl->SetIntensity(intensity);
        }
    }

    ImGui::End();
}


void Editor::DrawContentBrowser() {
    ImGui::Begin("Content Browser");
    ImGui::TextDisabled("Content browser coming soon");
    ImGui::End();
}

void Editor::DrawGizmo() {
    if (!m_selectedNode || m_state == State::Playing) return;
    if (!m_editorCamera || m_rmbLook) return;

    const glm::vec2 winSize = m_core.GetActiveWindow()->GetSize();
    ImGuizmo::SetRect(0, 0, winSize.x, winSize.y);

    glm::mat4 view  = m_editorCamera->GetViewMatrix();
    glm::mat4 proj  = m_editorCamera->GetProjectionMatrix();
    glm::mat4 world = m_selectedNode->GetWorldMatrix();
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
    if (const Node3d* parent = m_selectedNode->GetParent())
        localMatrix = glm::inverse(parent->GetWorldMatrix()) * world;

    glm::vec3 scale = {
        glm::length(glm::vec3(localMatrix[0])),
        glm::length(glm::vec3(localMatrix[1])),
        glm::length(glm::vec3(localMatrix[2]))
    };

    glm::mat3 rotMat = {
        glm::vec3(localMatrix[0]) / scale.x,
        glm::vec3(localMatrix[1]) / scale.y,
        glm::vec3(localMatrix[2]) / scale.z
    };

    m_selectedNode->SetPosition(glm::vec3(localMatrix[3]));
    m_selectedNode->SetRotation(glm::quat_cast(rotMat));
    m_selectedNode->SetScale(scale);
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