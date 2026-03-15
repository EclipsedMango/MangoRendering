cmake_minimum_required(VERSION 4.1)
project(MangoRendering)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

FetchContent_Declare(sdl GIT_REPOSITORY https://github.com/libsdl-org/SDL.git GIT_TAG release-3.2.10)
FetchContent_MakeAvailable(sdl)

FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.1)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(glad GIT_REPOSITORY https://github.com/Dav1dde/glad.git GIT_TAG glad2 SOURCE_SUBDIR cmake)
FetchContent_MakeAvailable(glad)
glad_add_library(glad_gl REPRODUCIBLE LOADER API gl:core=4.6)

FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG v1.92.6)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(stb GIT_REPOSITORY https://github.com/nothings/stb.git GIT_TAG master)
FetchContent_MakeAvailable(stb)

FetchContent_Declare(tinygltf GIT_REPOSITORY https://github.com/syoyo/tinygltf.git GIT_TAG v2.9.6)
FetchContent_MakeAvailable(tinygltf)

FetchContent_Declare(json GIT_REPOSITORY https://github.com/nlohmann/json.git GIT_TAG v3.11.3)
FetchContent_MakeAvailable(json)

add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC SDL3::SDL3 glad_gl)

add_executable(MangoRendering Src/main.cpp
        Src/Core/RenderApi.h
        Src/Core/RenderApi.cpp
        Src/Core/Window.h
        Src/Core/Window.cpp
        Src/Renderer/VertexArray.h
        Src/Renderer/VertexArray.cpp
        Src/Renderer/Mesh.h
        Src/Renderer/Mesh.cpp
        Src/Renderer/Shader.h
        Src/Renderer/Shader.cpp
        Src/Scene/Camera.cpp
        Src/Scene/Camera.h
        Src/Renderer/Texture.cpp
        Src/Renderer/Texture.h
        Src/Renderer/Buffers/UniformBuffer.cpp
        Src/Renderer/Buffers/UniformBuffer.h
        Src/Scene/Object.cpp
        Src/Scene/Object.h
        Src/Nodes/Transform.cpp
        Src/Nodes/Transform.h
        Src/Nodes/Lights/DirectionalLight.cpp
        Src/Nodes/Lights/DirectionalLight.h
        Src/Nodes/Lights/PointLight.cpp
        Src/Nodes/Lights/PointLight.h
        Src/Nodes/Lights/SpotLight.cpp
        Src/Nodes/Lights/SpotLight.h
        Src/Renderer/Buffers/ShaderStorageBuffer.cpp
        Src/Renderer/Buffers/ShaderStorageBuffer.h
        Src/Nodes/Lights/GpuLights.h
        Src/Renderer/Buffers/GpuBuffer.cpp
        Src/Renderer/Buffers/GpuBuffer.h
        Src/Renderer/GltfLoader.cpp
        Src/Renderer/GltfLoader.h
        Src/Renderer/Material.cpp
        Src/Renderer/Material.h
        Src/Renderer/Buffers/Framebuffer.cpp
        Src/Renderer/Buffers/Framebuffer.h
        Src/Scene/Frustum.h
        Src/Scene/Frustum.cpp
        Src/Renderer/CascadedShadowMap.cpp
        Src/Renderer/CascadedShadowMap.h
        Src/Renderer/PointLightShadowMap.cpp
        Src/Renderer/PointLightShadowMap.h
        Src/Renderer/Skybox.cpp
        Src/Renderer/Skybox.h
        Src/Nodes/Node3d.cpp
        Src/Nodes/Node3d.h
        Src/Core/Core.cpp
        Src/Core/Core.h
        Src/Renderer/Pipeline/ShadowRenderer.cpp
        Src/Renderer/Pipeline/ShadowRenderer.h
        Src/Renderer/Pipeline/LightManager.cpp
        Src/Renderer/Pipeline/LightManager.h
        Src/Renderer/Pipeline/ClusterSystem.cpp
        Src/Renderer/Pipeline/ClusterSystem.h
        Src/Nodes/MeshNode3d.cpp
        Src/Nodes/MeshNode3d.h
        Src/Nodes/RenderableNode3d.cpp
        Src/Nodes/RenderableNode3d.h
        Src/Nodes/Lights/LightNode3d.cpp
        Src/Nodes/Lights/LightNode3d.h
        Src/Nodes/Lights/DirectionalLightNode3d.cpp
        Src/Nodes/Lights/DirectionalLightNode3d.h
        Src/Nodes/Lights/PointLightNode3d.cpp
        Src/Nodes/Lights/PointLightNode3d.h
        Src/Nodes/Lights/SpotLightNode3d.cpp
        Src/Nodes/Lights/SpotLightNode3d.h)

target_link_libraries(MangoRendering SDL3::SDL3 glad_gl imgui ${CMAKE_DL_LIBS} nlohmann_json::nlohmann_json)
target_include_directories(MangoRendering PRIVATE ${CMAKE_SOURCE_DIR}/Src include ${glm_SOURCE_DIR} ${stb_SOURCE_DIR} ${tinygltf_SOURCE_DIR})

add_custom_target(copy_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Assets
        ${CMAKE_BINARY_DIR}/Assets
)

add_dependencies(MangoRendering copy_assets)