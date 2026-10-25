#pragma once
#include <string>

namespace mme {
    class Window;
}

// Dear ImGui'nin GLFW + OpenGL3 backend'leri (motor içi).
namespace mme::ImGuiBackend {
    void Init(Window& window, const std::string& fontPath, float fontSize);
    void Shutdown();
    void BeginFrame();
    void EndFrame();      // çizim verisini GPU'ya gönderir

    bool WantsMouse();    // fare bir ImGui penceresinin üstünde / sürükleniyor
    bool WantsKeyboard(); // bir ImGui metin kutusu odakta
}
