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
        Src/Renderer/stb.cpp
        Src/Renderer/Texture.cpp
        Src/Renderer/Texture.h
        Src/Buffers/UniformBuffer.cpp
        Src/Buffers/UniformBuffer.h
        Src/Scene/Object.cpp
        Src/Scene/Object.h
        Src/Scene/Transform.cpp
        Src/Scene/Transform.h
        Src/Lights/DirectionalLight.cpp
        Src/Lights/DirectionalLight.h
        Src/Lights/PointLight.cpp
        Src/Lights/PointLight.h
        Src/Lights/SpotLight.cpp
        Src/Lights/SpotLight.h
        Src/Buffers/ShaderStorageBuffer.cpp
        Src/Buffers/ShaderStorageBuffer.h
        Src/Lights/GpuLights.h
        Src/Buffers/GpuBuffer.cpp
        Src/Buffers/GpuBuffer.h)

target_link_libraries(MangoRendering SDL3::SDL3 glad_gl imgui ${CMAKE_DL_LIBS})
target_include_directories(MangoRendering PRIVATE ${CMAKE_SOURCE_DIR}/Src include ${glm_SOURCE_DIR} ${stb_SOURCE_DIR})

add_custom_target(copy_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Assets
        ${CMAKE_BINARY_DIR}/Assets
)

add_dependencies(MangoRendering copy_assets)