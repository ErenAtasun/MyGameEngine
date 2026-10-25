add_library(MakeMyEngine STATIC
    src/Log.cpp
    src/Window.cpp
    src/Input.cpp
    src/Application.cpp
    src/DebugOverlay.cpp
    src/Assets.cpp
    src/Renderer2D.cpp
    src/OrthoCamera.cpp
    src/OrthoCameraController.cpp
    src/Tilemap.cpp
    src/UI.cpp
    src/Scene.cpp
    src/Entity.cpp
    src/SpriteComponent.cpp
    src/RigidBody2D.cpp
    src/BoxCollider2D.cpp
    src/Physics2D.cpp
    src/ComponentRegistry.cpp
    src/SceneSerializer.cpp
    src/SpriteAnimator.cpp
    src/ParticleSystem.cpp
    src/ParticleEmitter.cpp
    src/Font.cpp
    src/Audio.cpp
    src/ImGuiBackend.cpp
    src/DebugPanel.cpp
)
add_library(MME::Engine ALIAS MakeMyEngine)

target_include_directories(MakeMyEngine
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${Stb_INCLUDE_DIR}
            ${MINIAUDIO_INCLUDE_DIR}
)

# glm ve ImGui public API'nin parçası (oyunlar kendi ImGui pencerelerini çizebilir);
# OpenGL/GLFW/miniaudio/json motorun iç detayı, oyun kodu görmez.
target_link_libraries(MakeMyEngine
    PUBLIC  glm::glm imgui::imgui
    PRIVATE glad::glad glfw OpenGL::GL nlohmann_json::nlohmann_json
)

# Debug derlemede asset'ler doğrudan repodan okunur (hot-reload, kaydedilen sahneler repoda kalır)
target_compile_definitions(MakeMyEngine PRIVATE MME_ASSETS_SOURCE_DIR="${MME_ASSETS_DIR}")

mme_set_warnings(MakeMyEngine)
