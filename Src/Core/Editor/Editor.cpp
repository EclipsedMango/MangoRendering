
#include "Editor.h"

#include "imgui_impl_sdl3.h"
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

void Editor::DrawViewport() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport");

    const ImVec2 viewportPos  = ImGui::GetCursorScreenPos();
    const ImVec2 viewportSize = ImGui::GetContentRegionAvail();

    if (viewportSize.x > 0 && viewportSize.y > 0) {
        const uint32_t w = static_cast<uint32_t>(viewportSize.x);
        const uint32_t h = static_cast<uint32_t>(viewportSize.y);

        m_core.GetRenderer().ResizeSceneFbo(w, h);
        if (m_editorCamera) {
            m_editorCamera->SetAspectRatio(viewportSize.x / viewportSize.y);
        }

        const ImTextureID texId = static_cast<ImTextureID>(static_cast<intptr_t>(m_core.GetRenderer().GetFinalScreenBuffer()));
        ImGui::Image(texId, viewportSize, ImVec2(0, 1), ImVec2(1, 0));

        m_viewportPos = viewportPos;
        m_viewportSize = viewportSize;

        DrawGizmo();
    }

    for (const auto node : m_sceneTree.GetSelectedNodes()) {
        glm::vec3 minB( FLT_MAX);
        glm::vec3 maxB(-FLT_MAX);

        glm::mat4 rootWorldInv = glm::inverse(node->GetWorldMatrix());
        ExpandLocalAABB(node, node, rootWorldInv, minB, maxB);

        const glm::vec3 localCenter = (minB + maxB) * 0.5f;
        const glm::vec3 half = (maxB - minB) * 0.5f + 0.05f;

        DrawWorldOBB(node->GetWorldMatrix(), localCenter, half, IM_COL32(255, 105, 5, 255));
    }

    if (!Input::IsMouseButtonHeld(SDL_BUTTON_RIGHT)) {
        m_viewportHovered = ImGui::IsWindowHovered();
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void Editor::Run() {
    uint64_t lastTime = SDL_GetTicksNS();

    while (m_core.GetActiveWindow()->IsOpen()) {
        const uint64_t now = SDL_GetTicksNS();
        const float deltaTime = (now - lastTime) / 1e9f;
        lastTime = now;

        const uint64_t cpuStart = SDL_GetTicksNS();

        m_core.PollEvents();

        if (Input::IsKeyJustPressed(SDL_SCANCODE_ESCAPE)) {
            m_sceneTree.ClearSelection();
        }

        if (Input::IsKeyJustPressed(SDL_SCANCODE_DELETE)) {
            m_sceneTree.DeleteSelectedNodes();
        }

        if (Input::IsKeyJustPressedWithMod(SDL_SCANCODE_D, SDL_SCANCODE_LCTRL)) {
            m_sceneTree.DuplicateSelectedNodes();
        }

        m_snapObjectMovement = Input::IsKeyHeld(SDL_SCANCODE_LCTRL) && !m_sceneTree.GetSelectedNodes().empty() && !Input::IsMouseButtonJustReleased(SDL_SCANCODE_LCTRL);

        Core::BeginImGuiFrame();
        if (m_rmbLook) {
            ImGui::GetIO().MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
        }

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
        DrawViewport();

        m_core.RenderScene();
        Core::EndImGuiFrame();

        const uint64_t cpuEnd = SDL_GetTicksNS();
        m_cpuTime = static_cast<float>(cpuEnd - cpuStart) / 1000000.0f;

        m_core.SwapBuffers();
    }
}

void Editor::UpdateEditorCamera(const float dt) {
    if (!m_editorCamera) return;

    const ImGuiIO& io = ImGui::GetIO();
    const bool imguiWantsKeys = io.WantCaptureKeyboard;

    const bool rmbHeld = Input::IsMouseButtonHeld(SDL_BUTTON_RIGHT);
    const bool shouldLook = rmbHeld && m_viewportHovered;

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

    if (m_rmbLook && m_viewportHovered) {
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
    // TODO: Make it draw a grid
    // ImGuizmo::DrawGrid()

    if (!m_sceneTree.GetSelectedNode() || m_sceneTree.GetSelectedNodes().empty() || m_state == State::Playing || !m_editorCamera) return;

    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
    ImGuizmo::SetRect(m_viewportPos.x, m_viewportPos.y, m_viewportSize.x, m_viewportSize.y);
    ImGuizmo::Enable(!m_rmbLook);
    ImGuizmo::SetGizmoSizeClipSpace(0.2f);

    glm::mat4 view = m_editorCamera->GetViewMatrix();
    glm::mat4 proj = m_editorCamera->GetProjectionMatrix();

    const std::vector<Node3d*> selectedNodes = m_sceneTree.GetSelectedNodes();

    glm::vec3 pivot(0.0f);
    for (const auto node : selectedNodes) {
        pivot += glm::vec3(node->GetWorldMatrix()[3]);
    }
    pivot /= static_cast<float>(selectedNodes.size());

    glm::mat4 pivotWorld = glm::translate(glm::mat4(1.0f), pivot);
    glm::mat4 delta = glm::mat4(1.0f);

    constexpr glm::vec3 scaleTranslationSnap = glm::vec3(0.5f);
    constexpr glm::vec3 rotationSnap = glm::vec3(45.0f);

    const glm::vec3* snap = nullptr;
    if (m_snapObjectMovement) {
        if (m_gizmoOp == ImGuizmo::TRANSLATE || m_gizmoOp == ImGuizmo::SCALE) {
            snap = &scaleTranslationSnap;
        } else if (m_gizmoOp == ImGuizmo::ROTATE) {
            snap = &rotationSnap;
        }
    }

    ImGuizmo::Manipulate(
        glm::value_ptr(view),
        glm::value_ptr(proj),
        m_gizmoOp,
        m_gizmoMode,
        glm::value_ptr(pivotWorld),
        glm::value_ptr(delta),
        m_snapObjectMovement ? glm::value_ptr(*snap) : nullptr
    );

    const bool isUsing = ImGuizmo::IsUsing();

    // snapshot scales the moment the user grabs the gizmo
    if (isUsing && !m_wasUsingGizmo) {
        m_gizmoInitialScales.clear();
        for (const auto node : selectedNodes) {
            m_gizmoInitialScales.push_back(node->GetScale());
        }
    }
    m_wasUsingGizmo = isUsing;

    if (!isUsing) return;

    if (m_gizmoOp == ImGuizmo::SCALE) {
        const glm::vec3 totalScale = {
            glm::length(glm::vec3(pivotWorld[0])),
            glm::length(glm::vec3(pivotWorld[1])),
            glm::length(glm::vec3(pivotWorld[2]))
        };

        for (size_t i = 0; i < selectedNodes.size(); i++) {
            selectedNodes[i]->SetScale(m_gizmoInitialScales[i] * totalScale);
        }

        return;
    }

    for (const auto node : selectedNodes) {
        glm::mat4 world = delta * node->GetWorldMatrix();

        glm::mat4 localMatrix = world;
        if (const Node3d* parent = node->GetParent()) {
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

        if (std::abs(glm::determinant(rotMat) - 1.0f) >= 0.01f) {
            rotMat[0] = glm::normalize(rotMat[0]);
            rotMat[1] = glm::normalize(glm::cross(glm::vec3(rotMat[2]), glm::vec3(rotMat[0])));
            rotMat[2] = glm::normalize(glm::cross(glm::vec3(rotMat[0]), glm::vec3(rotMat[1])));
        }

        node->SetRotation(glm::quat_cast(rotMat));
        node->SetPosition(glm::vec3(localMatrix[3]));
        node->SetScale(scale);
    }
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

void Editor::DrawWorldOBB(const glm::mat4& worldMatrix, const glm::vec3& localCenter, const glm::vec3& half, const ImU32 color) const {
    const glm::mat4 viewProj = m_editorCamera->GetProjectionMatrix() * m_editorCamera->GetViewMatrix();
    ImDrawList* list = ImGui::GetWindowDrawList();

    const glm::vec3 lc[8] = {
        localCenter + glm::vec3(-half.x,-half.y,-half.z),
        localCenter + glm::vec3( half.x,-half.y,-half.z),
        localCenter + glm::vec3( half.x, half.y,-half.z),
        localCenter + glm::vec3(-half.x, half.y,-half.z),
        localCenter + glm::vec3(-half.x,-half.y, half.z),
        localCenter + glm::vec3( half.x,-half.y, half.z),
        localCenter + glm::vec3( half.x, half.y, half.z),
        localCenter + glm::vec3(-half.x, half.y, half.z),
    };

    constexpr int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0},
        {4,5},{5,6},{6,7},{7,4},
        {0,4},{1,5},{2,6},{3,7}
    };

    ImVec2 screen[8];
    bool visible[8];
    for (int i = 0; i < 8; i++) {
        const glm::vec3 worldCorner = glm::vec3(worldMatrix * glm::vec4(lc[i], 1.0f));
        visible[i] = WorldToScreen(worldCorner, viewProj, screen[i]);
    }

    for (const auto& edge : edges) {
        if (visible[edge[0]] && visible[edge[1]]) {
            list->AddLine(screen[edge[0]], screen[edge[1]], color, 1.75f);
        }
    }
}

void Editor::ExpandLocalAABB(Node3d *root, const Node3d *node, glm::mat4 &rootWorldInv, glm::vec3 &minB, glm::vec3 &maxB) {
    constexpr glm::vec3 localCorners[8] = {
        {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f},
        {-0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f},
    };

    for (const auto& c : localCorners) {
        const glm::vec3 worldCorner = glm::vec3(node->GetWorldMatrix() * glm::vec4(c, 1.0f));
        const glm::vec3 rootLocal = glm::vec3(rootWorldInv * glm::vec4(worldCorner, 1.0f));
        minB = glm::min(minB, rootLocal);
        maxB = glm::max(maxB, rootLocal);
    }

    for (const auto child : node->GetChildren()) {
        ExpandLocalAABB(root, child, rootWorldInv, minB, maxB);
    }
}

void Editor::DrawWorldDirections() {
    ImDrawList* list = ImGui::GetBackgroundDrawList();

}

bool Editor::WorldToScreen(const glm::vec3 &worldPos, const glm::mat4 &viewProj, ImVec2 &out) const {
    const glm::vec4 clip = viewProj * glm::vec4(worldPos, 1.0f);

    if (clip.w <= 0.0f) {
        return false;
    }

    const glm::vec3 ndc = glm::vec3(clip) / clip.w;

    out.x = m_viewportPos.x + (ndc.x * 0.5f + 0.5f) * m_viewportSize.x;
    out.y = m_viewportPos.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * m_viewportSize.y;

    return true;
}
