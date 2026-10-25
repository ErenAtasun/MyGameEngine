#include <MME/Window.h>
#include <MME/Log.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cstdlib>

namespace mme {

    namespace {
        int s_WindowCount = 0;

        void GlfwErrorCallback(int code, const char* description) {
            MME_ERROR("GLFW hatasi ", code, ": ", description);
        }

        Window& FromNative(GLFWwindow* w) {
            return *static_cast<Window*>(glfwGetWindowUserPointer(w));
        }

        // İmleç konumu ekran koordinatında gelir; UI ve render framebuffer
        // pikseliyle çalıştığı için yüksek DPI ekranlarda ölçeklenmesi gerekir.
        void CursorToFramebuffer(GLFWwindow* w, double& x, double& y) {
            int winW = 0, winH = 0, fbW = 0, fbH = 0;
            glfwGetWindowSize(w, &winW, &winH);
            glfwGetFramebufferSize(w, &fbW, &fbH);
            if (winW > 0 && winH > 0) {
                x *= (double)fbW / winW;
                y *= (double)fbH / winH;
            }
        }
    }

    Window::Window(const WindowSpec& spec) {
        if (s_WindowCount == 0) {
            glfwSetErrorCallback(GlfwErrorCallback);
            if (!glfwInit()) {
                MME_ERROR("GLFW baslatilamadi");
                std::exit(EXIT_FAILURE);
            }
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_RESIZABLE, spec.resizable ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_VISIBLE, spec.visible ? GLFW_TRUE : GLFW_FALSE);
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

        m_Window = glfwCreateWindow(spec.width, spec.height, spec.title.c_str(), nullptr, nullptr);
        if (!m_Window) {
            MME_ERROR("Pencere olusturulamadi (OpenGL 3.3 destekleniyor mu?)");
            if (s_WindowCount == 0) glfwTerminate();
            std::exit(EXIT_FAILURE);
        }
        ++s_WindowCount;

        glfwMakeContextCurrent(m_Window);
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            MME_ERROR("GLAD OpenGL fonksiyonlarini yukleyemedi");
            std::exit(EXIT_FAILURE);
        }
        MME_INFO("OpenGL ", (const char*)glGetString(GL_VERSION), " | ", (const char*)glGetString(GL_RENDERER));

        SetVSync(spec.vsync);
        glfwGetFramebufferSize(m_Window, &m_FbWidth, &m_FbHeight);
        glfwSetWindowUserPointer(m_Window, this);
        InstallCallbacks();
    }

    Window::~Window() {
        if (m_Window) {
            glfwDestroyWindow(m_Window);
            if (--s_WindowCount == 0) glfwTerminate();
        }
    }

    void Window::InstallCallbacks() {
        glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* w) {
            Event e{ EventType::WindowClose };
            FromNative(w).Emit(e);
        });

        glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow* w, int width, int height) {
            Window& self = FromNative(w);
            self.m_FbWidth = width;
            self.m_FbHeight = height;
            Event e{ EventType::WindowResize };
            e.width = width;
            e.height = height;
            self.Emit(e);
        });

        glfwSetKeyCallback(m_Window, [](GLFWwindow* w, int key, int /*scancode*/, int action, int /*mods*/) {
            if (key == GLFW_KEY_UNKNOWN) return;
            Event e{ action == GLFW_RELEASE ? EventType::KeyReleased : EventType::KeyPressed };
            e.key = key;
            e.repeat = (action == GLFW_REPEAT);
            FromNative(w).Emit(e);
        });

        glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* w, int button, int action, int /*mods*/) {
            Event e{ action == GLFW_PRESS ? EventType::MouseButtonPressed : EventType::MouseButtonReleased };
            e.button = button;
            FromNative(w).Emit(e);
        });

        glfwSetCursorPosCallback(m_Window, [](GLFWwindow* w, double x, double y) {
            CursorToFramebuffer(w, x, y);
            Event e{ EventType::MouseMoved };
            e.x = (float)x;
            e.y = (float)y;
            FromNative(w).Emit(e);
        });

        glfwSetScrollCallback(m_Window, [](GLFWwindow* w, double xoff, double yoff) {
            Event e{ EventType::MouseScrolled };
            e.x = (float)xoff;
            e.y = (float)yoff;
            FromNative(w).Emit(e);
        });
    }

    void Window::PollEvents() { glfwPollEvents(); }
    void Window::WaitEvents(double timeoutSeconds) { glfwWaitEventsTimeout(timeoutSeconds); }
    void Window::SwapBuffers() { glfwSwapBuffers(m_Window); }
    void Window::SetVSync(bool enabled) { glfwSwapInterval(enabled ? 1 : 0); }
    void Window::SetTitle(const std::string& title) { glfwSetWindowTitle(m_Window, title.c_str()); }
    double Window::GetTime() const { return glfwGetTime(); }
}
