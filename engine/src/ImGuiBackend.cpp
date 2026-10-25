#include "ImGuiBackend.h"
#include <MME/Log.h>
#include <MME/Window.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <filesystem>

namespace mme::ImGuiBackend {

    namespace {
        bool s_Ready = false;

        void ApplyStyle() {
            ImGui::StyleColorsDark();
            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowRounding = 6.0f;
            style.FrameRounding = 4.0f;
            style.GrabRounding = 4.0f;
            style.WindowBorderSize = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 0.92f;
        }
    }

    void Init(Window& window, const std::string& fontPath, float fontSize) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = "imgui.ini"; // pencere konumları .exe'nin yanında saklanır
        ApplyStyle();

        // Türkçe karakterler için Latin-1 + Latin Extended-A
        static const ImWchar kRanges[] = { 0x0020, 0x00FF, 0x0100, 0x017F, 0 };
        if (!fontPath.empty() && std::filesystem::exists(fontPath))
            io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize, nullptr, kRanges);
        else
            io.Fonts->AddFontDefault();

        // install_callbacks = true: ImGui, Window'un GLFW callback'lerini zincirler
        ImGui_ImplGlfw_InitForOpenGL(window.GetNativeHandle(), true);
        ImGui_ImplOpenGL3_Init("#version 330 core");
        s_Ready = true;
    }

    void Shutdown() {
        if (!s_Ready) return;
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        s_Ready = false;
    }

    void BeginFrame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void EndFrame() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    bool WantsMouse() { return s_Ready && ImGui::GetIO().WantCaptureMouse; }
    bool WantsKeyboard() { return s_Ready && ImGui::GetIO().WantCaptureKeyboard; }
}
