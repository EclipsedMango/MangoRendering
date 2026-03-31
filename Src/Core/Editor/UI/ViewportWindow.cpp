
#include "ViewportWindow.h"

#include <utility>

#include "Core/Input.h"
#include "Core/Editor/Editor.h"
#include "Core/Editor/EditorCameraController.h"
#include "Core/Editor/Utils/DebugDraw.h"
#include "glm/glm.hpp"
#include "Nodes/CameraNode3d.h"

ViewportWindow::ViewportWindow(Editor* editor, std::string  name) : m_editor(editor), m_name(std::move(name)) {
    const glm::vec2 winSize = m_editor->GetCore().GetActiveWindow()->GetSize();
    const float aspect = winSize.y != 0.0f ? winSize.x / winSize.y : 1.0f;

    m_camera = std::make_unique<CameraNode3d>(glm::vec3(0, 2, 6), 75.0f, aspect);
    m_cameraController = std::make_unique<EditorCameraController>(m_camera.get(), m_editor->GetCore().GetActiveWindow());

    m_framebuffer = std::make_unique<Framebuffer>(
        static_cast<uint32_t>(winSize.x),
        static_cast<uint32_t>(winSize.y),
        FramebufferType::ColorDepth
    );
}

ViewportWindow::~ViewportWindow() = default;

void ViewportWindow::LoadScene(std::unique_ptr<Node3d> scene) {
    if (m_scene) {
        Core::UnregisterScene(m_scene.get());
    }

    m_scene = std::move(scene);
    if (m_scene) {
        m_editor->GetCore().RegisterScene(m_scene.get());
    }
}

std::unique_ptr<Node3d> ViewportWindow::DetachScene() {
    if (m_scene) {
        Core::UnregisterScene(m_scene.get());
    }

    return std::move(m_scene);
}

void ViewportWindow::SaveSnapshot(std::unique_ptr<Node3d> scene) {
    m_snapshot = std::move(scene);
}

std::unique_ptr<Node3d> ViewportWindow::TakeSnapshot() {
    return std::move(m_snapshot);
}

// gets root node of scene
Node3d* ViewportWindow::GetScene() const {
    if (m_scene) return m_scene.get();
    return nullptr;
}

void ViewportWindow::Update(const float deltaTime) {
    m_timeSinceLastRender += deltaTime;

    if (m_editor->GetState() != Editor::State::Playing) {
        m_cameraController->Update(deltaTime, m_viewportHovered);
    }
}

void ViewportWindow::Draw() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin(m_name.c_str());

    const ImVec2 viewportPos = ImGui::GetCursorScreenPos();
    const ImVec2 viewportSize = ImGui::GetContentRegionAvail();

    if (viewportSize.x > 0 && viewportSize.y > 0) {
        const uint32_t w = static_cast<uint32_t>(viewportSize.x);
        const uint32_t h = static_cast<uint32_t>(viewportSize.y);

        bool shouldRender = true;

        if (!m_viewportHovered && !m_cameraController->IsLooking()) {
            if (m_timeSinceLastRender < 0.1f) {
                shouldRender = false;
            }
        }

        if (m_framebuffer->GetWidth() != w || m_framebuffer->GetHeight() != h) {
            m_framebuffer->Resize(w, h);
            shouldRender = true;
        }

        m_camera->SetAspectRatio(viewportSize.x / viewportSize.y);

        Node3d* activeScene = m_editor->GetState() == Editor::State::Playing ? m_editor->GetCore().GetScene() : GetScene();

        if (shouldRender) {
            const CameraNode3d* renderCam = m_camera.get();
            if (m_editor->GetState() == Editor::State::Playing) {
                if (const CameraNode3d* gameCam = m_editor->GetCore().GetGameCamera()) {
                    renderCam = gameCam;
                }
            }

            m_editor->GetCore().RenderScene(activeScene, renderCam, m_framebuffer.get());
            m_timeSinceLastRender = 0.0f;
        }

        const ImTextureID texId = static_cast<ImTextureID>(static_cast<intptr_t>(m_framebuffer->GetColorAttachment()));
        ImGui::Image(texId, viewportSize, ImVec2(0, 1), ImVec2(1, 0));

        m_viewportPos = viewportPos;
        m_viewportSize = viewportSize;

        const bool isPlaying = m_editor->GetState() == Editor::State::Playing;

        std::vector<Node3d*> validSelectedNodes;
        for (auto node : m_editor->GetSceneTree().GetSelectedNodes()) {
            if (Core::IsInScene(node, activeScene)) {
                validSelectedNodes.push_back(node);
            }
        }

        m_editor->GetGizmoSystem().UpdateAndDraw(
            m_camera.get(),
            validSelectedNodes,
            m_viewportPos,
            m_viewportSize,
            isPlaying,
            m_cameraController->IsLooking()
        );
    }

    const Node3d* activeScene = m_editor->GetState() == Editor::State::Playing ? m_editor->GetCore().GetScene() : GetScene();
    const glm::mat4 viewProj = m_camera->GetProjectionMatrix() * m_camera->GetViewMatrix();

    for (const auto node : m_editor->GetSceneTree().GetSelectedNodes()) {
        if (!Core::IsInScene(node, activeScene)) continue;

        glm::vec3 minB( FLT_MAX);
        glm::vec3 maxB(-FLT_MAX);

        glm::mat4 rootWorldInv = glm::inverse(node->GetWorldMatrix());
        DebugDraw::ExpandLocalAABB(node, node, rootWorldInv, minB, maxB);

        const glm::vec3 localCenter = (minB + maxB) * 0.5f;
        const glm::vec3 half = (maxB - minB) * 0.5f + 0.05f;

        DebugDraw::DrawWorldOBB(viewProj, node->GetWorldMatrix(), localCenter, half, IM_COL32(255, 105, 5, 255), m_viewportPos, m_viewportSize);
    }

    if (!Input::IsMouseButtonHeld(SDL_BUTTON_RIGHT)) {
        m_viewportHovered = ImGui::IsWindowHovered();
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void ViewportWindow::DrawContent() {
    const ImVec2 viewportPos = ImGui::GetCursorScreenPos();
    const ImVec2 viewportSize = ImGui::GetContentRegionAvail();

    if (viewportSize.x > 0 && viewportSize.y > 0) {
        const uint32_t w = static_cast<uint32_t>(viewportSize.x);
        const uint32_t h = static_cast<uint32_t>(viewportSize.y);

        bool shouldRender = true;

        if (!m_viewportHovered && !m_cameraController->IsLooking()) {
            if (m_timeSinceLastRender < 0.1f) shouldRender = false;
        }

        if (m_framebuffer->GetWidth() != w || m_framebuffer->GetHeight() != h) {
            m_framebuffer->Resize(w, h);
            shouldRender = true;
        }

        m_camera->SetAspectRatio(viewportSize.x / viewportSize.y);

        Node3d* activeScene = m_editor->GetState() == Editor::State::Playing ? m_editor->GetCore().GetScene() : GetScene();

        if (shouldRender) {
            const CameraNode3d* renderCam = m_camera.get();
            if (m_editor->GetState() == Editor::State::Playing) {
                if (const CameraNode3d* gameCam = m_editor->GetCore().GetGameCamera()) {
                    renderCam = gameCam;
                }
            }

            m_editor->GetCore().RenderScene(activeScene, renderCam, m_framebuffer.get());
            m_timeSinceLastRender = 0.0f;
        }

        const ImTextureID texId = static_cast<ImTextureID>(static_cast<intptr_t>(m_framebuffer->GetColorAttachment()));
        ImGui::Image(texId, viewportSize, ImVec2(0, 1), ImVec2(1, 0));

        m_viewportPos = viewportPos;
        m_viewportSize = viewportSize;

        std::vector<Node3d*> validSelectedNodes;
        for (auto node : m_editor->GetSceneTree().GetSelectedNodes()) {
            if (Core::IsInScene(node, activeScene)) {
                validSelectedNodes.push_back(node);
            }
        }

        m_editor->GetGizmoSystem().UpdateAndDraw(
            m_camera.get(),
            validSelectedNodes,
            m_viewportPos, m_viewportSize,
            m_editor->GetState() == Editor::State::Playing,
            m_cameraController->IsLooking()
        );
    }

    const Node3d* activeScene = m_editor->GetState() == Editor::State::Playing ? m_editor->GetCore().GetScene() : GetScene();
    const glm::mat4 viewProj = m_camera->GetProjectionMatrix() * m_camera->GetViewMatrix();

    for (const auto node : m_editor->GetSceneTree().GetSelectedNodes()) {
        if (!Core::IsInScene(node, activeScene)) continue;

        glm::vec3 minB( FLT_MAX);
        glm::vec3 maxB(-FLT_MAX);

        glm::mat4 rootWorldInv = glm::inverse(node->GetWorldMatrix());
        DebugDraw::ExpandLocalAABB(node, node, rootWorldInv, minB, maxB);

        const glm::vec3 localCenter = (minB + maxB) * 0.5f;
        const glm::vec3 half = (maxB - minB) * 0.5f + 0.05f;

        DebugDraw::DrawWorldOBB(viewProj, node->GetWorldMatrix(), localCenter, half, IM_COL32(255, 105, 5, 255), m_viewportPos, m_viewportSize);
    }

    if (!Input::IsMouseButtonHeld(SDL_BUTTON_RIGHT)) {
        m_viewportHovered = ImGui::IsWindowHovered();
    }
}
