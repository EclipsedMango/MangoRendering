//
// #include "Core.h"
//
// #include <iostream>
//
// #include "imgui.h"
// #include "imgui_impl_opengl3.h"
// #include "imgui_impl_sdl3.h"
// #include "RenderApi.h"
//
// Core::Core(Node3d* scene) {
//     m_currentScene = scene;
// }
//
// Core::~Core() {
//     delete m_currentScene;
// }
//
// void Core::Init() {
//     InitRenderer();
//     InitImGui();
// }
//
// void Core::InitRenderer() {
//     RenderApi::Init();
//     m_activeWindow = RenderApi::CreateWindow("Mango", {500, 500}, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
//
//     std::cout << "Vendor: "   << glGetString(GL_VENDOR)   << std::endl;
//     std::cout << "Renderer: " << glGetString(GL_RENDERER)  << std::endl;
//     std::cout << "Version: "  << glGetString(GL_VERSION)   << std::endl;
//
//     SDL_GL_SetSwapInterval(0);
//     SDL_SetWindowRelativeMouseMode(m_activeWindow->GetSDLWindow(), true);
//
//     m_activeWindow->MakeCurrent();
// }
//
// void Core::InitImGui() const {
//     IMGUI_CHECKVERSION();
//     ImGui::CreateContext();
//     ImGui_ImplSDL3_InitForOpenGL(m_activeWindow->GetSDLWindow(), m_activeWindow->GetContext());
//     ImGui_ImplOpenGL3_Init("#version 460");
// }
//
// void Core::BeginImGuiFrame() {
//     ImGui_ImplOpenGL3_NewFrame();
//     ImGui_ImplSDL3_NewFrame();
//     ImGui::NewFrame();
// }
//
// void Core::EndImGuiFrame() {
//     ImGui::Render();
//     ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
// }
//
// void Core::Process() const {
//     constexpr float FIXED_TIMESTEP = 1.0f / 60.0f;
//     float accumulator = 0.0f;
//     uint64_t lastTime = SDL_GetTicksNS();
//
//     while (m_activeWindow->IsOpen()) {
//         const uint64_t now = SDL_GetTicksNS();
//         const float deltaTime = (now - lastTime) / 1e9f;
//         lastTime = now;
//
//         accumulator += deltaTime;
//         while (accumulator >= FIXED_TIMESTEP) {
//             TickNodes(m_currentScene, FIXED_TIMESTEP);
//             accumulator -= FIXED_TIMESTEP;
//         }
//
//         ProcessNodes(m_currentScene);
//
//         BeginImGuiFrame();
//
//         RenderApi::ClearColour({0.16f, 0.16f, 0.16f, 1.0f});
//         RenderApi::Flush();
//
//         EndImGuiFrame();
//
//         m_activeWindow->SwapBuffers();
//     }
// }
//
// void Core::TickNodes(Node3d *node, const float deltaTime) {
//     node->PhysicsProcess(deltaTime);
//     for (const auto child : node->GetChildren()) {
//         TickNodes(child, deltaTime);
//     }
// }
//
// void Core::ProcessNodes(Node3d *node) {
//     node->Process();
//     for (const auto child : node->GetChildren()) {
//         ProcessNodes(child);
//     }
// }
//
// void Core::ChangeScene(Node3d* scene) {
//     delete m_currentScene;
//     m_currentScene = scene;
// }
