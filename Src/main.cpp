#include "Core/Core.h"
#include "Core/ResourceManager.h"
#include "Core/Editor/Editor.h"
#include "Nodes/MeshNode3d.h"
#include "Nodes/PortalNode3d.h"
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

    auto texture = ResourceManager::Get().Load<Texture>("face.png");
    auto teddyTexture = ResourceManager::Get().Load<Texture>("sky_16_2k.png");
    auto shader = ResourceManager::Get().LoadShader("default", "test.vert", "test.frag");
    auto portalShader = ResourceManager::Get().LoadShader("portal_mask", "portal_mask.vert", "portal_mask.frag");

    auto quad = std::make_unique<MeshNode3d>();
    quad->SetMeshByName("Quad");
    quad->SetScale({4.0f, 4.0f, 4.0f});
    quad->GetActiveMaterial()->SetDoubleSided(true);
    quad->GetActiveMaterial()->SetDiffuse("red_brick_03_diff_1k.png");
    quad->GetActiveMaterial()->SetAmbientOcclusion("red_brick_03_ao_1k.png");
    quad->GetActiveMaterial()->SetDisplacement("red_brick_03_disp_1k.png");
    quad->GetActiveMaterial()->SetNormal("red_brick_03_nor_gl_1k.png");
    quad->GetActiveMaterial()->SetRoughness("red_brick_03_rough_1k.png");
    liveScene->AddChild(std::move(quad));

    auto plane = std::make_unique<MeshNode3d>();
    plane->SetMeshByName("Plane");
    plane->SetRotationEuler({0, 0, 0});
    plane->SetScale({100.0f, 1.0f, 100.0f});
    plane->SetPosition({0, -5, 0});
    liveScene->AddChild(std::move(plane));

    auto cube2 = std::make_unique<MeshNode3d>();
    cube2->SetMeshByName("Cube");
    cube2->SetPosition({-20.0, -4.0, 0});
    cube2->SetScale({1.0f, 2.0f, 100.0f});
    liveScene->AddChild(std::move(cube2));

    const std::string housePath = ResourceManager::Get().ResolveAssetPath("stylised_sky_player_home_dioroma.glb");
    auto house = std::unique_ptr(GltfLoader::Load(housePath, shader));
    house->SetScale({0.15f, 0.15f, 0.15f});
    house->SetPosition({0, -4, -15});
    liveScene->AddChild(std::move(house));

    const std::string dancingPath = ResourceManager::Get().ResolveAssetPath("Silly Dancing.glb");
    auto dancing = std::unique_ptr(GltfLoader::Load(dancingPath, shader));
    dancing->SetScale({0.15f, 0.15f, 0.15f});
    liveScene->AddChild(std::move(dancing));

    auto cube1 = std::make_unique<MeshNode3d>();
    cube1->SetMeshByName("Cube");
    cube1->SetPosition({0, -4.9, 2});
    cube1->SetScale({0.2f, 0.2f, 0.2f});
    cube1->GetActiveMaterial()->SetMetallicValue(1.0);
    cube1->GetActiveMaterial()->SetRoughnessValue(0.5);
    liveScene->AddChild(std::move(cube1));

    auto sphere = std::make_unique<MeshNode3d>();
    sphere->SetMeshByName("Sphere");
    sphere->GetActiveMaterial()->SetMetallicValue(0.75);
    sphere->GetActiveMaterial()->SetRoughnessValue(0.25);
    sphere->SetPosition({0, 4, 0});

    for (int i = 0; i < 30; i++) {
        for (int j = 0; j < 30; j++) {
            auto newSphere = sphere->Clone();
            newSphere->SetPosition({i+0.5, 4, j+0.5});
            liveScene->AddChild(std::move(newSphere));
        }
    }

    liveScene->AddChild(std::move(sphere));

    auto portalA = std::make_unique<PortalNode3d>();
    portalA->SetMeshByName("Quad");
    portalA->SetPosition({0, 0, 0.1f});
    portalA->SetScale({1.5f, 2.5f, 1.0f});
    portalA->GetActiveMaterial()->SetShader(portalShader);
    PortalNode3d* portalAPtr = portalA.get();
    liveScene->AddChild(std::move(portalA));

    auto portalB = std::make_unique<PortalNode3d>();
    portalB->SetMeshByName("Quad");
    portalB->SetPosition({3.5f, 6.25f, -5.5f});
    portalB->SetRotationEuler({180, -30, 180});
    portalB->SetScale({1.5f, 2.5f, 1.0f});
    portalB->GetActiveMaterial()->SetShader(portalShader);
    PortalNode3d* portalBPtr = portalB.get();
    liveScene->AddChild(std::move(portalB));

    PortalNode3d::LinkPair(portalAPtr, portalBPtr);

    auto sun = std::make_unique<DirectionalLightNode3d>(glm::vec3(0.5f, -0.6f, -0.5f), glm::vec3(0.9f, 0.65f, 0.32f), 2.5f);
    liveScene->AddChild(std::move(sun));

    auto pointLight = std::make_unique<PointLightNode3d>(glm::vec3(2, 2, -2), glm::vec3(0.6f, 0.7f, 0.9f), 1.0f, 15.0f);
    liveScene->AddChild(std::move(pointLight));

    auto skybox = std::make_unique<SkyboxNode3d>(ResourceManager::Get().ResolveAssetPath("kloppenheim_06_puresky_1k.hdr"));
    editor.GetCore().SetGlobalSkybox(std::move(skybox));

    auto camera = std::make_unique<CameraNode3d>(glm::vec3(0.0f, 1.0f, 8.0f), 75.0f, 1280.0f / 720.0f);
    camera->SetScript("Root/hello.lua");
    camera->SetAsGameCamera(true);
    liveScene->AddChild(std::move(camera));

    editor.Run();

    scene.reset();
    shader.reset();
    portalShader.reset();
    texture.reset();
    teddyTexture.reset();

    return 0;
}
