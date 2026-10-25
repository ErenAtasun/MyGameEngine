#pragma once
#include <MME/Event.h>
#include <functional>
#include <string>

struct GLFWwindow;

namespace mme {

    struct WindowSpec {
        std::string title = "MakeMyEngine";
        int width = 1280;     // istenen istemci alanı (ekran koordinatı)
        int height = 720;
        bool vsync = true;
        bool resizable = true;
        bool visible = true;  // false: gizli pencere (araçlar, otomatik testler)
    };

    // GLFW penceresi + OpenGL 3.3 core context. GLFW olaylarını motor
    // olaylarına çevirip tek bir callback'e iletir.
    class Window {
    public:
        using EventCallback = std::function<void(Event&)>;

        explicit Window(const WindowSpec& spec);
        ~Window();
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        void PollEvents();
        void WaitEvents(double timeoutSeconds); // simge durumundayken CPU'yu boşa yakmamak için
        void SwapBuffers();

        void SetVSync(bool enabled);
        void SetTitle(const std::string& title);
        void SetEventCallback(EventCallback cb) { m_Callback = std::move(cb); }

        // Framebuffer boyutu (piksel). Simge durumunda 0 olabilir.
        int GetWidth() const { return m_FbWidth; }
        int GetHeight() const { return m_FbHeight; }
        bool IsMinimized() const { return m_FbWidth == 0 || m_FbHeight == 0; }

        double GetTime() const;
        GLFWwindow* GetNativeHandle() const { return m_Window; }

    private:
        void Emit(Event& e) { if (m_Callback) m_Callback(e); }
        void InstallCallbacks();

        GLFWwindow* m_Window = nullptr;
        int m_FbWidth = 0, m_FbHeight = 0;
        EventCallback m_Callback;
    };
}
