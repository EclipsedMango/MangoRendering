cmake_minimum_required(VERSION 4.1)
project(MangoRendering)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

FetchContent_Declare(sdl GIT_REPOSITORY https://github.com/libsdl-org/SDL.git GIT_TAG release-2.30.0)
FetchContent_MakeAvailable(sdl)

FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.1)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(glad GIT_REPOSITORY https://github.com/Dav1dde/glad.git GIT_TAG glad2 SOURCE_SUBDIR cmake)
FetchContent_MakeAvailable(glad)
glad_add_library(glad_gl REPRODUCIBLE LOADER API gl:core=4.6)

FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG v1.91.1)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC SDL2::SDL2 glad_gl)

add_executable(MangoRendering main.cpp
        RenderApi.h
        RenderApi.cpp
        Window.h
        Window.cpp
        GpuBuffer.h
        GpuBuffer.cpp
        Mesh.h
        Mesh.cpp
        Shader.h
        Shader.cpp
        Camera.cpp
        Camera.h)

target_link_libraries(MangoRendering SDL2::SDL2 SDL2::SDL2main glad_gl imgui ${CMAKE_DL_LIBS})
target_include_directories(MangoRendering PRIVATE include ${glm_SOURCE_DIR})

add_custom_target(copy_shaders ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Shaders
        ${CMAKE_BINARY_DIR}/Shaders
)
add_dependencies(MangoRendering copy_shaders)