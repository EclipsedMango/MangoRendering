
#include "Editor.h"

#include <functional>

#include "../Input.h"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.inl"
#include "Nodes/CameraNode3d.h"

Editor::Editor(Node3d* scene) : m_core(scene), m_inspector(this), m_sceneTree(this) {
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

        uint64_t cpuStart = SDL_GetTicksNS();

        m_core.PollEvents();

        if (Input::IsKeyJustPressed(SDL_SCANCODE_DELETE)) {
            m_sceneTree.DeleteSelectedNodes();
        }

        if (Input::IsKeyJustPressedWithMod(SDL_SCANCODE_D, SDL_SCANCODE_LCTRL)) {
            m_sceneTree.DuplicateSelectedNodes();
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
        m_sceneTree.DrawSceneTree(m_core.GetScene());
        m_inspector.DrawInspector(m_sceneTree.GetSelectedNode());
        DrawContentBrowser();

        m_core.RenderScene();
        Core::EndImGuiFrame();

        uint64_t cpuEnd = SDL_GetTicksNS();
        m_cpuTime = static_cast<float>(cpuEnd - cpuStart) / 1000000.0f;

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

    const float fps = ImGui::GetIO().Framerate;
    ImGui::Text("FPS: %.0f (%.2f ms) | CPU: %.2f ms", fps, 1000.0f / fps, m_cpuTime);

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

void Editor::DrawContentBrowser() {
    ImGui::Begin("Content Browser");
    ImGui::TextDisabled("Content browser coming soon");
    ImGui::End();
}

void Editor::DrawGizmo() {
    ImGuizmo::BeginFrame();

    if (!m_sceneTree.GetSelectedNode() || m_state == State::Playing || !m_editorCamera) return;

    const glm::vec2 winSize = m_core.GetActiveWindow()->GetSize();
    ImGuizmo::SetRect(0, 0, winSize.x, winSize.y);
    ImGuizmo::Enable(!m_rmbLook);

    glm::mat4 view  = m_editorCamera->GetViewMatrix();
    glm::mat4 proj  = m_editorCamera->GetProjectionMatrix();
    glm::mat4 world = m_sceneTree.GetSelectedNode()->GetWorldMatrix();
    glm::mat4 delta = glm::mat4(1.0f);

    ImGuizmo::Manipulate(
        glm::value_ptr(view),
        glm::value_ptr(proj),
        m_gizmoOp,
        m_gizmoMode,
        glm::value_ptr(world),
        glm::value_ptr(delta),
        m_snapObjectMovement ? glm::value_ptr(glm::vec3(0.5)) : nullptr
    );

    if (!ImGuizmo::IsUsing()) return;

    glm::mat4 localMatrix = world;
    if (const Node3d* parent = m_sceneTree.GetSelectedNode()->GetParent()) {
        localMatrix = glm::inverse(parent->GetWorldMatrix()) * world;
    }

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
        m_sceneTree.GetSelectedNode()->SetRotation(glm::quat_cast(rotMat));
    } else {
        rotMat[0] = glm::normalize(rotMat[0]);
        rotMat[1] = glm::normalize(glm::cross(glm::vec3(rotMat[2]), glm::vec3(rotMat[0])));
        rotMat[2] = glm::normalize(glm::cross(glm::vec3(rotMat[0]), glm::vec3(rotMat[1])));
        m_sceneTree.GetSelectedNode()->SetRotation(glm::quat_cast(rotMat));
    }

    m_sceneTree.GetSelectedNode()->SetPosition(glm::vec3(localMatrix[3]));
    m_sceneTree.GetSelectedNode()->SetScale(scale);
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
