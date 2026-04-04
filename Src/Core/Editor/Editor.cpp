
#include "Editor.h"

#include <iostream>

#include "EditorCameraController.h"
#include "Utils/DebugDraw.h"
#include "../Input.h"
#include "Core/PackedScene.h"
#include "Core/ResourceManager.h"
#include "glm/gtc/type_ptr.inl"
#include "UI/ViewportWindow.h"

Editor::Editor(std::unique_ptr<Node3d> scene) : m_inspector(this), m_sceneTree(this), m_contentBrowserWindow(this) {
    m_core.Init();

    // setup user project assets and engine assets
    const std::filesystem::path engineAssets = ResourceManager::Get().GetEngineDirectory();
    const std::filesystem::path userProjectRoot = engineAssets.parent_path() / "Root";
    const std::filesystem::path userAssets = userProjectRoot / "Assets";

    ResourceManager::Get().SetUserDirectory(userAssets);

    m_mainViewport = std::make_unique<ViewportWindow>(this, "Main Viewport");
    m_mainViewport->LoadScene(std::move(scene));

    m_activeViewport = m_mainViewport.get();
    m_viewports.push_back(std::move(m_mainViewport));

    m_core.SetEditorCamera(m_activeViewport->GetCamera());
    m_core.SetCameraMode(Core::CameraMode::Editor);

    m_sceneTabs.push_back({ "main" });

    SDL_SetWindowRelativeMouseMode(m_core.GetActiveWindow()->GetSDLWindow(), false);
    Input::SetMouseDeltaEnabled(false);
}

Editor::~Editor() {
    for (auto& viewport : m_viewports) {
        if (viewport) {
            viewport.reset();
        }
    }

    SDL_SetWindowRelativeMouseMode(m_core.GetActiveWindow()->GetSDLWindow(), false);
    Input::SetMouseDeltaEnabled(false);
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

        if (Input::IsKeyJustPressed(SDL_SCANCODE_F2) && m_sceneTree.IsHoveringSceneTree()) {
            if (!m_sceneTree.GetSelectedNodes().empty() && m_sceneTree.GetSelectedNodes().size() < 2) {
                Node3d* sel = m_sceneTree.GetSelectedNode();
                if (sel) {
                    m_sceneTree.BeginRename(sel);
                }
            }
        }

        if (Input::IsKeyJustPressedWithMod(SDL_SCANCODE_D, SDL_SCANCODE_LCTRL)) {
            m_sceneTree.DuplicateSelectedNodes();
        }

        const bool isLooking = IsAnyViewportLooking();

        m_gizmoSystem.HandleShortcuts(isLooking);
        Core::BeginImGuiFrame();

        if (isLooking) {
            ImGui::GetIO().MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
        }

        if (m_state != State::Playing) {
            std::vector<Node3d*> scenesToUpdate;
            if (m_core.GetScene()) scenesToUpdate.push_back(m_core.GetScene());

            for (const auto& viewport : m_viewports) {
                viewport->Update(deltaTime);
                Node3d* s = viewport->GetScene();
                if (s && std::ranges::find(scenesToUpdate, s) == scenesToUpdate.end()) {
                    scenesToUpdate.push_back(s);
                }
            }

            for (auto* s : scenesToUpdate) {
                s->UpdateWorldTransform();
            }
        } else {
            m_core.StepFrame(deltaTime);
        }

        DrawMenuBar();
        DrawViewportTabs();

        if (m_activeViewport->GetCameraController()->GetMoveSpeedLastFrame() != m_activeViewport->GetCameraController()->GetMoveSpeed()) {
            m_activeViewport->GetCameraController()->SetMoveSpeedLastFrame(m_activeViewport->GetCameraController()->GetMoveSpeed());
            m_speedIndicatorTimer = kSpeedIndicatorFadeTime;
        }

        if (m_speedIndicatorTimer > 0.0f) {
            m_speedIndicatorTimer -= deltaTime;
            DrawCameraSpeedIndication(m_speedIndicatorTimer / kSpeedIndicatorFadeTime);
        }

        Node3d* sceneToDraw = m_state == State::Playing ? m_core.GetScene() : m_activeViewport->GetScene();
        m_sceneTree.DrawSceneTree(sceneToDraw);

        m_inspector.DrawInspector(m_sceneTree.GetSelectedNode());
        m_contentBrowserWindow.DrawContentBrowser();

        if (Input::IsKeyJustPressed(SDL_SCANCODE_DELETE)) {
            m_sceneTree.DeleteSelectedNodes();
        }

        Core::EndImGuiFrame();

        const uint64_t cpuEnd = SDL_GetTicksNS();
        m_cpuTime = static_cast<float>(cpuEnd - cpuStart) / 1000000.0f;

        m_core.SwapBuffers();
    }
}

void Editor::DrawViewportTabs() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewports", nullptr);

    if (ImGui::BeginTabBar("##viewport_tabs", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs)) {
        for (int i = 0; i < static_cast<int>(m_viewports.size()); i++) {
            bool open = true;
            const std::string& name = m_viewports[i]->GetName();

            if (ImGui::BeginTabItem(name.c_str(), &open)) {
                if (m_activeViewport != m_viewports[i].get()) {
                    m_activeViewport = m_viewports[i].get();
                    m_core.SetEditorCamera(m_activeViewport->GetCamera());
                }

                m_viewports[i]->DrawContent();
                ImGui::EndTabItem();
            }

            if (!open && m_viewports.size() > 1) {
                if (m_activeViewport == m_viewports[i].get()) {
                    const int next = i > 0 ? i - 1 : 1;
                    m_activeViewport = m_viewports[next].get();
                    m_core.SetEditorCamera(m_activeViewport->GetCamera());
                }

                m_viewports[i]->DetachScene();
                m_viewports.erase(m_viewports.begin() + i);
                m_sceneTree.ClearSelection();
                i--;
            }
        }

        if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing)) {
            const std::string name = "Viewport " + std::to_string(m_viewports.size() + 1);
            auto vp = std::make_unique<ViewportWindow>(this, name);

            auto newScene = std::make_unique<Node3d>();
            newScene->SetName("New Scene");

            vp->LoadScene(std::move(newScene));

            m_viewports.push_back(std::move(vp));
            m_activeViewport = m_viewports.back().get();
            m_core.SetEditorCamera(m_activeViewport->GetCamera());
            m_sceneTree.ClearSelection();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void Editor::DrawMenuBar() {
    if (!ImGui::BeginMainMenuBar()) return;

    const float fps = ImGui::GetIO().Framerate;
    ImGui::Text("FPS: %.0f (%.2f ms) | CPU: %.2f ms", fps, 1000.0f / fps, m_cpuTime);

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New Scene"))  { /* TODO */ }
        if (ImGui::MenuItem("Open Scene")) {
            try {
                const auto packed = PackedScene::LoadFromFile("john.yml");
                auto loaded = packed.Instantiate();
                m_activeViewport->LoadScene(std::move(loaded));
                m_sceneTree.ClearSelection();
            } catch (const std::exception& e) {
                std::cerr << "[Editor] Failed to open scene: " << e.what() << "\n";
            }
        }
        if (ImGui::MenuItem("Save Scene")) {
            const Node3d* activeScene = m_state == State::Playing ? m_core.GetScene() : m_activeViewport->GetScene();
            if (activeScene) {
                PackedScene scene(activeScene);
                scene.SaveToFile("john.yml");
            }
        }
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
    const float spacing = 4.0f;
    const float totalWidth = buttonWidth * 3 + spacing * 2;
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f + ImGui::GetCursorPosX());

    const bool isPlaying = m_state == State::Playing;
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

void Editor::DrawCameraSpeedIndication(const float alpha) const {
    const ImVec2 pos  = m_activeViewport->GetViewportPos();
    const ImVec2 size = m_activeViewport->GetViewportSize();
    ImDrawList* dl = ImGui::GetForegroundDrawList();

    const float a = alpha * alpha;
    const auto  A = [&](const int base) { return static_cast<int>(base * a); };

    constexpr float barW = 12.0f;
    constexpr float pad = 16.0f;
    constexpr float barH = 256.0f;

    const ImVec2 barMax = ImVec2(pos.x + size.x - pad, pos.y + size.y - pad);
    const ImVec2 barMin = ImVec2(barMax.x - barW, barMax.y - barH);

    dl->AddRectFilled(barMin, barMax, IM_COL32(0, 0, 0, A(160)), 4.0f);

    constexpr float minSpeed = 0.05f;
    constexpr float maxSpeed = 550.0f;
    const float speed = m_activeViewport->GetCameraController()->GetMoveSpeed();

    float t = (glm::log(speed) - glm::log(minSpeed)) / (glm::log(maxSpeed) - glm::log(minSpeed));
    t = glm::clamp(t, 0.02f, 0.98f);

    const float fillTop = barMin.y + barH * (1.0f - t);

    dl->PushClipRect(barMin, barMax, true);
    dl->AddRectFilled(ImVec2(barMin.x, fillTop), barMax, IM_COL32(136, 33, 191, A(210)), 4.0f);

    dl->AddRectFilled(ImVec2(barMin.x, fillTop), ImVec2(barMax.x, fillTop + 2.0f), IM_COL32(172, 50, 237, A(255)));
    dl->PopClipRect();

    dl->AddRect(barMin, barMax, IM_COL32(255, 255, 255, A(60)), 4.0f, ImDrawFlags_RoundCornersAll, 1.0f);
}

void Editor::OnPlay() {
    m_sceneTree.ClearSelection();

    for (const auto& vp : m_viewports) {
        if (Node3d* scene = vp->GetScene()) {
            vp->SaveSnapshot(scene->Clone());
        }
    }

    auto mainScene = m_viewports.front()->DetachScene();

    if (!mainScene) {
        mainScene = std::make_unique<Node3d>();
        mainScene->SetName("FallbackRoot");
    }

    for (size_t i = 1; i < m_viewports.size(); ++i) {
        if (auto scene = m_viewports[i]->DetachScene()) {
            mainScene->AddChild(std::move(scene));
        }
    }

    m_core.ChangeScene(std::move(mainScene));

    if (CameraNode3d* cam = FindGameCamera(m_core.GetScene())) {
        m_core.SetGameCamera(cam);
    }

    m_core.SetCameraMode(Core::CameraMode::Game);
    m_state = State::Playing;

    for (const auto& vp : m_viewports) {
        vp->GetCameraController()->CancelLook();
    }

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
    m_sceneTree.ClearSelection();
    m_core.ChangeScene(nullptr);

    for (const auto& vp : m_viewports) {
        if (auto snapshot = vp->TakeSnapshot()) {
            vp->LoadScene(std::move(snapshot));
        }
    }

    m_core.SetCameraMode(Core::CameraMode::Editor);
    m_state = State::Editing;

    for (const auto& vp : m_viewports) {
        vp->GetCameraController()->CancelLook();
    }

    SDL_SetWindowRelativeMouseMode(m_core.GetActiveWindow()->GetSDLWindow(), false);
    Input::SetMouseDeltaEnabled(false);
}

CameraNode3d * Editor::FindGameCamera(Node3d *node) {
    if (auto* cam = dynamic_cast<CameraNode3d*>(node); cam && cam->IsGameCamera()) {
        return cam;
    }

    for (Node3d* child : node->GetChildren()) {
        if (auto* found = FindGameCamera(child)) {
            return found;
        }
    }

    return nullptr;
}

bool Editor::IsAnyViewportLooking() const {
    for (const auto& vp : m_viewports) {
        if (vp->GetCameraController()->IsLooking()) {
            return true;
        }
    }

    return false;
}
