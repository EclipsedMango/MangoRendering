#include "Core/Core.h"
#include "Core/ResourceManager.h"
#include "Core/Editor/Editor.h"
#include "Nodes/Lights/DirectionalLightNode3d.h"
#include "Renderer/Meshes/GltfLoader.h"

int main() {
    auto scene = std::make_unique<Node3d>();
    Editor editor(std::move(scene));

    Node3d* liveScene = editor.GetActiveViewport()->GetScene();

    // auto shader = ResourceManager::Get().LoadShader("Default", "Engine://Shaders/default.vert", "Engine://Shaders/default.frag");
    //
    // const std::string dancingPath = ResourceManager::Get().ResolveAssetPath("Engine://Models/SillyDancing/Silly Dancing.glb");
    // auto dancing = std::unique_ptr(GltfLoader::Load(dancingPath, shader));
    // dancing->SetScale({0.10f, 0.10f, 0.10f});
    // liveScene->AddChild(std::move(dancing));

    auto sun = std::make_unique<DirectionalLightNode3d>(glm::vec3(0.5f, -0.6f, -0.5f), glm::vec3(0.9f, 0.65f, 0.32f), 2.5f);
    liveScene->AddChild(std::move(sun));

    auto skybox = std::make_unique<SkyboxNode3d>(ResourceManager::Get().ResolveAssetPath("Engine://Textures/Cubemaps/kloppenheim_06_puresky_1k.hdr"));
    editor.GetCore().SetGlobalSkybox(std::move(skybox));

    auto camera = std::make_unique<CameraNode3d>(glm::vec3(0.0f, 1.0f, 8.0f), 75.0f, 1280.0f / 720.0f);
    camera->SetScript("Root://hello.lua");
    camera->SetAsGameCamera(true);
    liveScene->AddChild(std::move(camera));

    editor.Run();

    scene.reset();

    return 0;
}
