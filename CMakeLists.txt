cmake_minimum_required(VERSION 4.1)
project(MangoRendering)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

include(FetchContent)

FetchContent_Declare(sdl GIT_REPOSITORY https://github.com/libsdl-org/SDL.git GIT_TAG release-3.2.10)
set(SDL_TEST_LIBRARY OFF)
set(SDL_TESTS OFF)
FetchContent_MakeAvailable(sdl)

FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.1)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(glad GIT_REPOSITORY https://github.com/Dav1dde/glad.git GIT_TAG glad2 SOURCE_SUBDIR cmake)
FetchContent_MakeAvailable(glad)
glad_add_library(glad_gl REPRODUCIBLE LOADER API gl:core=4.6)

FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG docking)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(imguizmo GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git GIT_TAG master)
FetchContent_MakeAvailable(imguizmo)

FetchContent_Declare(stb GIT_REPOSITORY https://github.com/nothings/stb.git GIT_TAG master)
FetchContent_MakeAvailable(stb)

FetchContent_Declare(tinygltf GIT_REPOSITORY https://github.com/syoyo/tinygltf.git GIT_TAG v2.9.6)
FetchContent_MakeAvailable(tinygltf)

FetchContent_Declare(fkYAML GIT_REPOSITORY https://github.com/fktn-k/fkYAML.git GIT_TAG v0.4.2)
FetchContent_MakeAvailable(fkYAML)

FetchContent_Declare(lua GIT_REPOSITORY https://github.com/marovira/lua.git GIT_TAG 5.4.8)
FetchContent_MakeAvailable(lua)

FetchContent_Declare(sol2 GIT_REPOSITORY https://github.com/ThePhD/sol2.git GIT_TAG develop)
FetchContent_MakeAvailable(sol2)

set(PHYSICS_REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(TARGET_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(TARGET_HELLO_WORLD OFF CACHE BOOL "" FORCE)
set(TARGET_PERFORMANCE_TEST OFF CACHE BOOL "" FORCE)
set(TARGET_SAMPLES OFF CACHE BOOL "" FORCE)
set(TARGET_VIEWER OFF CACHE BOOL "" FORCE)
FetchContent_Declare(JoltPhysics GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git GIT_TAG v5.3.0 SOURCE_SUBDIR Build)
FetchContent_MakeAvailable(JoltPhysics)

add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp
)

target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends ${imguizmo_SOURCE_DIR})
target_link_libraries(imgui PUBLIC SDL3::SDL3 glad_gl)

add_executable(MangoRendering Src/main.cpp
        Src/Core/RenderApi.h
        Src/Core/RenderApi.cpp
        Src/Core/Window.h
        Src/Core/Window.cpp
        Src/Renderer/VertexArray.h
        Src/Renderer/VertexArray.cpp
        Src/Renderer/Meshes/Mesh.h
        Src/Renderer/Meshes/Mesh.cpp
        Src/Renderer/Shader.h
        Src/Renderer/Shader.cpp
        Src/Renderer/Animation/Animator.h
        Src/Renderer/Animation/Animator.cpp
        Src/Renderer/Animation/Skeleton.h
        Src/Renderer/Animation/Skeleton.cpp
        Src/Renderer/Materials/Texture.cpp
        Src/Renderer/Materials/Texture.h
        Src/Renderer/Buffers/UniformBuffer.cpp
        Src/Renderer/Buffers/UniformBuffer.h
        Src/Renderer/Lights/DirectionalLight.cpp
        Src/Renderer/Lights/DirectionalLight.h
        Src/Renderer/Lights/PointLight.cpp
        Src/Renderer/Lights/PointLight.h
        Src/Renderer/Lights/SpotLight.cpp
        Src/Renderer/Lights/SpotLight.h
        Src/Renderer/Buffers/ShaderStorageBuffer.cpp
        Src/Renderer/Buffers/ShaderStorageBuffer.h
        Src/Renderer/Lights/GpuLights.h
        Src/Renderer/Buffers/GpuBuffer.cpp
        Src/Renderer/Buffers/GpuBuffer.h
        Src/Renderer/Meshes/GltfLoader.cpp
        Src/Renderer/Meshes/GltfLoader.h
        Src/Renderer/Materials/Material.cpp
        Src/Renderer/Materials/Material.h
        Src/Renderer/Buffers/Framebuffer.cpp
        Src/Renderer/Buffers/Framebuffer.h
        Src/Scene/Frustum.h
        Src/Scene/Frustum.cpp
        Src/Renderer/Shadows/CascadedShadowMap.cpp
        Src/Renderer/Shadows/CascadedShadowMap.h
        Src/Renderer/Shadows/PointLightShadowMap.cpp
        Src/Renderer/Shadows/PointLightShadowMap.h
        Src/Renderer/Skybox.cpp
        Src/Renderer/Skybox.h
        Src/Nodes/Node3d.cpp
        Src/Nodes/Node3d.h
        Src/Nodes/AnimationControllerNode3d.cpp
        Src/Nodes/AnimationControllerNode3d.h
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
        Src/Nodes/Lights/SpotLightNode3d.h
        Src/Core/TreeListener.cpp
        Src/Core/TreeListener.h
        Src/Nodes/CameraNode3d.cpp
        Src/Nodes/CameraNode3d.h
        Src/Core/Input.cpp
        Src/Core/Input.h
        Src/Nodes/SkyboxNode3d.cpp
        Src/Nodes/SkyboxNode3d.h
        Src/Core/Editor/Editor.cpp
        Src/Core/Editor/Editor.h
        Src/Renderer/Meshes/PrimitiveMesh.cpp
        Src/Renderer/Meshes/PrimitiveMesh.h
        Src/Core/Editor/UI/InspectorPanel.cpp
        Src/Core/Editor/UI/InspectorPanel.h
        Src/Core/Editor/UI/SceneTreePanel.cpp
        Src/Core/Editor/UI/SceneTreePanel.h
        Src/Renderer/Pipeline/IBLPrecomputer.cpp
        Src/Renderer/Pipeline/IBLPrecomputer.h
        Src/Renderer/Materials/EquirectToCubemap.cpp
        Src/Renderer/Materials/EquirectToCubemap.h
        Src/Renderer/Meshes/CubeGeometry.h
        Src/Core/PackedScene.cpp
        Src/Core/PackedScene.h
        Src/Core/PropertyHolder.h
        Src/Core/ResourceManager.h
        Src/Core/NodeRegistry.h
        Src/Core/PropertyRegistry.h
        Src/Core/PropertyHolder.cpp
        Src/Core/Editor/EditorStyle.cpp
        Src/Core/Editor/EditorStyle.h
        Src/Core/Editor/Utils/DebugDraw.cpp
        Src/Core/Editor/Utils/DebugDraw.h
        Src/Core/Editor/EditorCameraController.cpp
        Src/Core/Editor/EditorCameraController.h
        Src/Core/Editor/GizmoSystem.cpp
        Src/Core/Editor/GizmoSystem.h
        Src/Core/Editor/UI/ContentBrowserWindow.cpp
        Src/Core/Editor/UI/ContentBrowserWindow.h
        Src/Core/Editor/UI/ViewportWindow.cpp
        Src/Core/Editor/UI/ViewportWindow.h
        Src/Core/Editor/UI/MenuBar.cpp
        Src/Core/Editor/UI/MenuBar.h
        Src/Nodes/PortalNode3d.cpp
        Src/Nodes/PortalNode3d.h
        Src/Nodes/CollisionShape3d.cpp
        Src/Nodes/CollisionShape3d.h
        Src/Nodes/RigidBody3d.cpp
        Src/Nodes/RigidBody3d.h
        Src/Core/ResourceManager.cpp
        Src/Core/ScriptManager.cpp
        Src/Core/ScriptManager.h
        Src/Core/PhysicsWorld.cpp
        Src/Core/PhysicsWorld.h
)

target_link_libraries(MangoRendering PRIVATE SDL3::SDL3 glad_gl imgui ${CMAKE_DL_LIBS} fkYAML::fkYAML lua::lua sol2::sol2 Jolt)
target_compile_definitions(MangoRendering PRIVATE SOL_LUA_VERSION=504)

set_property(TARGET MangoRendering PROPERTY CXX_SCAN_FOR_MODULES OFF)
set_property(TARGET imgui PROPERTY CXX_SCAN_FOR_MODULES OFF)

target_include_directories(MangoRendering PRIVATE
        ${CMAKE_SOURCE_DIR}/Src
        ${CMAKE_SOURCE_DIR}/include
        ${glm_SOURCE_DIR}
        ${stb_SOURCE_DIR}
        ${tinygltf_SOURCE_DIR}
)

add_custom_target(copy_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/EngineAssets
        ${CMAKE_BINARY_DIR}/EngineAssets
)

add_custom_target(copy_root ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Root
        ${CMAKE_BINARY_DIR}/Root
)

add_custom_command(TARGET MangoRendering POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3>
        $<TARGET_FILE_DIR:MangoRendering>
)

add_dependencies(MangoRendering copy_assets copy_root)
