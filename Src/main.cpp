
#include "Core/Core.h"
#include "Core/ResourceManager.h"
#include "Core/Editor/Editor.h"
#include "Nodes/MeshNode3d.h"
#include "Nodes/PortalNode3d.h"
#include "Renderer/Shader.h"
#include "Nodes/Lights/DirectionalLightNode3d.h"
#include "Nodes/Lights/PointLightNode3d.h"
#include "Renderer/Meshes/GltfLoader.h"
#include "Renderer/Meshes/PrimitiveMesh.h"

int main() {
    auto scene = std::make_unique<Node3d>();
    Editor editor(std::move(scene));

    Node3d* liveScene = editor.GetActiveViewport()->GetScene();

    const auto cubeMesh = std::make_shared<CubeMesh>();
    const auto sphereMesh = std::make_shared<SphereMesh>();

    auto texture = std::make_shared<Texture>("../Assets/Textures/face.png");
    auto teddyTexture = std::make_shared<Texture>("../Assets/Textures/Cubemaps/sky_16_2k/sky_16_2k.png");
    const auto shader = ResourceManager::Get().LoadShader("default", "../Assets/Shaders/test.vert", "../Assets/Shaders/test.frag");
    const auto portalShader = ResourceManager::Get().LoadShader("portal_mask", "../Assets/Shaders/portal_mask.vert", "../Assets/Shaders/portal_mask.frag");

    // Node3d* teddy = GltfLoader::Load("../Assets/Models/teddy.glb", shader);
    // teddy->SetPosition({0, 0, 5});
    // liveScene->AddChild(teddy);

    auto quad = std::make_unique<MeshNode3d>(shader);
    quad->SetMeshByName("Quad");
    quad->SetScale({4.0f, 4.0f, 4.0f});
    quad->GetActiveMaterial().SetDoubleSided(true);
    quad->GetActiveMaterial().SetDiffuse("../Assets/Textures/RedBrick/red_brick_03_diff_1k.png");
    quad->GetActiveMaterial().SetAmbientOcclusion("../Assets/Textures/RedBrick/red_brick_03_ao_1k.png");
    quad->GetActiveMaterial().SetDisplacement("../Assets/Textures/RedBrick/red_brick_03_disp_1k.png");
    quad->GetActiveMaterial().SetNormal("../Assets/Textures/RedBrick/red_brick_03_nor_gl_1k.png");
    quad->GetActiveMaterial().SetRoughness("../Assets/Textures/RedBrick/red_brick_03_rough_1k.png");
    liveScene->AddChild(std::move(quad));

    auto house = std::unique_ptr<Node3d>(GltfLoader::Load("../Assets/Models/PlayerHome/stylised_sky_player_home_dioroma.glb", shader));
    house->SetScale({0.15f, 0.15f, 0.15f});
    house->SetPosition({0, -4, -15});
    liveScene->AddChild(std::move(house));

    auto cube1 = std::make_unique<MeshNode3d>(shader);
    cube1->SetMeshByName("Cube");
    cube1->SetPosition({0, -4.9, 2});
    cube1->SetScale({0.2f, 0.2f, 0.2f});
    cube1->GetActiveMaterial().SetMetallicValue(1.0);
    cube1->GetActiveMaterial().SetRoughnessValue(0.5);
    liveScene->AddChild(std::move(cube1));

    auto sphere = std::make_unique<MeshNode3d>(shader);
    sphere->SetMeshByName("Sphere");
    sphere->GetActiveMaterial().SetMetallicValue(0.75);
    sphere->GetActiveMaterial().SetRoughnessValue(0.25);
    sphere->SetPosition({0, 4, -2});
    liveScene->AddChild(std::move(sphere));

    auto portalA = std::make_unique<PortalNode3d>();
    portalA->SetMeshByName("Quad");
    portalA->SetPosition({0, 0, 0.1f});
    portalA->SetScale({1.5f, 2.5f, 1.0f});
    portalA->SetShader(portalShader);
    PortalNode3d* portalAPtr = portalA.get();
    liveScene->AddChild(std::move(portalA));

    auto portalB = std::make_unique<PortalNode3d>();
    portalB->SetMeshByName("Quad");
    portalB->SetPosition({3.5f, 6.25f, -5.5f});
    portalB->SetRotationEuler({180, -30, 180});
    portalB->SetScale({1.5f, 2.5f, 1.0f});
    portalB->SetShader(portalShader);
    PortalNode3d* portalBPtr = portalB.get();
    liveScene->AddChild(std::move(portalB));

    PortalNode3d::LinkPair(portalAPtr, portalBPtr);

    auto sun = std::make_unique<DirectionalLightNode3d>(glm::vec3(0.5f, -0.6f, -0.5f), glm::vec3(0.9f, 0.65f, 0.32f), 2.5f);
    liveScene->AddChild(std::move(sun));

    auto pointLight = std::make_unique<PointLightNode3d>(glm::vec3(2, 2, -2), glm::vec3(0.6f, 0.7f, 0.9f), 1.0f, 15.0f);
    liveScene->AddChild(std::move(pointLight));

    auto skybox = std::make_unique<SkyboxNode3d>("../Assets/Textures/Cubemaps/kloppenheim_06_puresky_1k.hdr");
    editor.GetCore().SetGlobalSkybox(std::move(skybox));

    auto camera = std::make_unique<CameraNode3d>(glm::vec3(0.0f, 1.0f, 8.0f), 75.0f, 1280.0f / 720.0f);
    CameraNode3d* gameCamera = camera.get();
    liveScene->AddChild(std::move(camera));

    editor.GetCore().SetGameCamera(gameCamera);
    editor.GetCore().SetCameraMode(Core::CameraMode::Game);

    editor.Run();

    scene.reset();
    texture.reset();
    teddyTexture.reset();

    return 0;
}
