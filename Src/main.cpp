
#include "Core/Core.h"
#include "Core/ResourceManager.h"
#include "Core/Editor/Editor.h"
#include "Nodes/MeshNode3d.h"
#include "Renderer/Shader.h"
#include "Nodes/Lights/DirectionalLightNode3d.h"
#include "Nodes/Lights/PointLightNode3d.h"
#include "Renderer/Meshes/GltfLoader.h"
#include "Renderer/Meshes/PrimitiveMesh.h"

int main() {
    // build scene
    Node3d* scene = new Node3d();

    Editor editor(scene);

    const auto cubeMesh = std::make_shared<CubeMesh>();
    const auto sphereMesh = std::make_shared<SphereMesh>();

    auto texture = std::make_shared<Texture>("../Assets/Textures/face.png");
    auto teddyTexture = std::make_shared<Texture>("../Assets/Textures/Cubemaps/sky_16_2k/sky_16_2k.png");
    const auto shader = ResourceManager::Get().LoadShader("default", "../Assets/Shaders/test.vert", "../Assets/Shaders/test.frag");

    Node3d* teddy = GltfLoader::Load("../Assets/Models/teddy.glb", shader);
    teddy->SetPosition({0, 0, 5});
    scene->AddChild(teddy);

    Node3d* house = GltfLoader::Load("../Assets/Models/stylised_sky_player_home_dioroma.glb", shader);
    house->SetScale({0.15f, 0.15f, 0.15f});
    house->SetPosition({0, -4, -4});
    scene->AddChild(house);

    MeshNode3d* cube1 = new MeshNode3d(shader);
    cube1->SetMeshByName("Cube");
    cube1->SetPosition({0, -4.9, 2});
    cube1->SetScale({0.2f, 0.2f, 0.2f});
    cube1->GetActiveMaterial().SetMetallicValue(1.0);
    cube1->GetActiveMaterial().SetRoughnessValue(0.5);
    scene->AddChild(cube1);

    MeshNode3d* sphere = new MeshNode3d(shader);
    sphere->SetMeshByName("Sphere");
    sphere->GetActiveMaterial().SetMetallicValue(0.75);
    sphere->GetActiveMaterial().SetRoughnessValue(0.25);
    sphere->SetPosition({0, 4, -2});
    scene->AddChild(sphere);

    for (int i = 0; i < 100; i++) {
        MeshNode3d* cube = new MeshNode3d(shader);
        cube->SetMeshByName("Cube");
        cube->SetPosition({
            (i % 10) * 2.0f - 9.0f,
            -4.9f,
            (i / 10) * 2.0f
        });
        cube->SetScale({0.3f, 0.3f, 0.3f});
        scene->AddChild(cube);
    }

    DirectionalLightNode3d* sun = new DirectionalLightNode3d({0.5f, -0.6f, -0.5f}, {0.9f, 0.65f, 0.32f}, 2.5f);
    scene->AddChild(sun);

    PointLightNode3d* pointLight = new PointLightNode3d({2, 2, -2}, {0.6f, 0.7f, 0.9f}, 1.0f, 15.0f);
    scene->AddChild(pointLight);

    SkyboxNode3d* skybox = new SkyboxNode3d("../Assets/Textures/Cubemaps/kloppenheim_06_puresky_1k.hdr");
    scene->AddChild(skybox);

    CameraNode3d* camera = new CameraNode3d({0, 0, 3}, 75.0f, 500.0f / 500.0f);
    editor.GetCore().SetGameCamera(camera);
    scene->AddChild(camera);

    editor.Run();

    texture.reset();
    teddyTexture.reset();

    return 0;
}
