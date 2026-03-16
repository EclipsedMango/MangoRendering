
#include <iostream>
#include <random>

#include "Core/Core.h"
#include "Core/Editor.h"
#include "Nodes/MeshNode3d.h"
#include "Renderer/Meshes/Mesh.h"
#include "Renderer/Shader.h"
#include "Nodes/Lights/DirectionalLightNode3d.h"
#include "Nodes/Lights/PointLightNode3d.h"
#include "Renderer/Meshes/GltfLoader.h"

int main() {
    // build scene
    Node3d* scene = new Node3d();

    Editor editor(scene);

    Mesh* mesh = new Mesh(
        {
            // Back face
            {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
            // Front face
            {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
            {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
            // Left face
            {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
            {{-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
            {{-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
            // Right face
            {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
            // Bottom face
            {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
            {{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
            // Top face
            {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
            {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        },{
            1,  0,  2,  2,  0,  3,
            4,  5,  6,  6,  7,  4,
            8,  9,  10, 10, 11, 8,
            13, 12, 14, 14, 12, 15,
            16, 17, 18, 18, 19, 16,
            21, 20, 22, 22, 20, 23
        }
    );
    mesh->Upload();

    auto texture = std::make_shared<Texture>("../Assets/Textures/face.png");
    auto teddyTexture = std::make_shared<Texture>("../Assets/Textures/Cubemaps/sky_16_2k/sky_16_2k.png");
    Shader* shader = new Shader("../Assets/Shaders/test.vert", "../Assets/Shaders/test.frag");

    Node3d* teddy = GltfLoader::Load("../Assets/Models/teddy.glb", shader);
    for (auto& children : teddy->GetChildren()) {
        if (const auto meshNode = dynamic_cast<MeshNode3d*>(children)) {
            meshNode->GetActiveMaterial().SetDiffuse(teddyTexture);
        }
    }

    teddy->SetPosition({0, 0, 0});
    scene->AddChild(teddy);

    MeshNode3d* floor = new MeshNode3d(mesh, shader);
    floor->SetPosition({0, -5, 0});
    floor->SetScale({100, 0.5f, 100});
    floor->GetActiveMaterial().SetDiffuse(texture);
    scene->AddChild(floor);

    MeshNode3d* cube1 = new MeshNode3d(mesh, shader);
    cube1->SetPosition({0, -2.5f, 2});
    cube1->GetActiveMaterial().SetMetallicValue(1.0);
    scene->AddChild(cube1);

    MeshNode3d* cube2 = new MeshNode3d(mesh, shader);
    cube2->SetPosition({0, -2.5f, -2});
    scene->AddChild(cube2);

    DirectionalLightNode3d* sun = new DirectionalLightNode3d({0.5f, -0.6f, -0.5f}, {0.9f, 0.65f, 0.32f}, 0.1f);
    scene->AddChild(sun);

    PointLightNode3d* pointLight = new PointLightNode3d({2, 2, -2}, {0.6f, 0.7f, 0.9f}, 1.0f, 15.0f);
    scene->AddChild(pointLight);

    SkyboxNode3d* skybox = new SkyboxNode3d({
        "../Assets/Textures/Cubemaps/sky_16_2k/sky_16_cubemap_2k/px.png",
        "../Assets/Textures/Cubemaps/sky_16_2k/sky_16_cubemap_2k/nx.png",
        "../Assets/Textures/Cubemaps/sky_16_2k/sky_16_cubemap_2k/py.png",
        "../Assets/Textures/Cubemaps/sky_16_2k/sky_16_cubemap_2k/ny.png",
        "../Assets/Textures/Cubemaps/sky_16_2k/sky_16_cubemap_2k/pz.png",
        "../Assets/Textures/Cubemaps/sky_16_2k/sky_16_cubemap_2k/nz.png",
    });
    scene->AddChild(skybox);

    // random
    std::mt19937 rng(42);
    auto randFloat = [&](const float min, const float max) {
        return std::uniform_real_distribution(min, max)(rng);
    };

    constexpr int NUM_OBJECTS = 50;
    for (int i = 0; i < NUM_OBJECTS; i++) {
        MeshNode3d* obj = new MeshNode3d(mesh, shader);
        obj->SetPosition({ randFloat(-30, 30), randFloat(-5, 10), randFloat(-30, 30) });
        obj->SetScale({ randFloat(0.3f, 2.0f), randFloat(0.3f, 2.0f), randFloat(0.3f, 2.0f) });
        obj->GetActiveMaterial().SetDiffuse(texture);
        scene->AddChild(obj);
    }

    constexpr int NUM_LIGHTS = 10;
    for (int i = 0; i < NUM_LIGHTS; i++) {
        PointLightNode3d* light = new PointLightNode3d(
            { randFloat(-30, 30), randFloat(0, 8), randFloat(-30, 30) },
            { randFloat(0.1f, 1.0f), randFloat(0.1f, 1.0f), randFloat(0.1f, 1.0f) },
            randFloat(0.1f, 1.0f),
            randFloat(8.0f, 20.0f)
        );
        light->SetAttenuation(1.0f, 0.22f, 0.20f);
        scene->AddChild(light);
    }

    CameraNode3d* camera = new CameraNode3d({0, 0, 3}, 75.0f, 500.0f / 500.0f);
    editor.GetCore().SetGameCamera(camera);
    scene->AddChild(camera);

    editor.Run();

    texture.reset();
    teddyTexture.reset();
    delete mesh;
    delete shader;

    return 0;
}
