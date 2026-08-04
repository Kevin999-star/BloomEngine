cmake_minimum_required(VERSION 3.20)

set(CMAKE_TOOLCHAIN_FILE "C:/Users/Kevin/vcpkg/scripts/buildsystems/vcpkg.cmake")

project(MyEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(directx-headers CONFIG REQUIRED)
find_package(directx12-agility CONFIG REQUIRED)
find_package(gameinput CONFIG REQUIRED)
find_package(dstorage CONFIG REQUIRED)
find_package(ms-gdk CONFIG REQUIRED)

add_executable(MyEngine
    main.cpp

    Core/Application.cpp
    Core/Time.cpp
    Core/Camera.cpp

    Platform/Window.cpp
    Platform/Input.cpp

    Renderer/Shader.cpp
    Renderer/VertexArray.cpp
    Renderer/VertexBuffer.cpp
    Renderer/IndexBuffer.cpp
    Renderer/Renderer.cpp
    Renderer/RendererContext.cpp
    Renderer/Texture.cpp
    Renderer/RendererLine.cpp
    Renderer/Material.cpp
    Renderer/Mesh.cpp
    Editor/Gizmos.cpp
    Engine/Graphics/DX12/Core/RendererDX12.cpp
    Engine/Graphics/DX12/Core/DeviceResources.cpp
    Scene/Entity.cpp
    Scene/Scene.cpp
    External/tinyobjloader/tiny_obj_loader.h
    External/tinyobjloader/tiny_obj_loader.cc

    
    
)

set(IMGUI_SRC
    External/ImGui/imgui.cpp
    External/ImGui/imgui_draw.cpp
    External/ImGui/imgui_widgets.cpp
    External/ImGui/imgui_tables.cpp
    External/ImGui/imgui_impl_glfw.cpp
    External/ImGui/imgui_impl_opengl3.cpp
)

target_sources(MyEngine PRIVATE ${IMGUI_SRC})
target_sources(MyEngine PRIVATE External/ImGui/ImGuizmo.cpp)

target_link_libraries(MyEngine PRIVATE
    d3d12
    dxgi
    dxguid
    Microsoft::DirectX-Headers
    Microsoft::DirectX12-Agility
    Microsoft::GameInput
    Microsoft::DirectStorage
    Xbox::GameRuntime
    Xbox::HTTPClient
    Xbox::XCurl
    Xbox::XSAPI
    Xbox::GameChat2

    glfw
    glad::glad
)


target_include_directories(MyEngine PRIVATE
    External
    External/tinyobjloader        # ⭐ AÑADIDO
    External/ImGui
    Engine/Graphics/DX12/Core
    Scene
)



