#include <MME/Application.h>
#include <MME/Assets.h>
#include <MME/Audio.h>
#include <MME/ComponentRegistry.h>
#include <MME/Font.h>
#include <MME/Log.h>
#include <MME/Renderer2D.h>
#include <MME/UI.h>
#include "DebugOverlay.h"
#include "DebugPanel.h"
#include "ImGuiBackend.h"
#include "InputInternal.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <filesystem>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace mme {

    Application* Application::s_Instance = nullptr;

    namespace {
        // Göreli asset yolları, uygulama nereden başlatılırsa başlatılsın
        // .exe'nin yanındaki klasöre göre çözülsün.
        void SetWorkingDirectoryToExecutable() {
#ifdef _WIN32
            wchar_t path[MAX_PATH];
            const DWORD len = GetModuleFileNameW(nullptr, path, MAX_PATH);
            if (len > 0 && len < MAX_PATH)
                std::filesystem::current_path(std::filesystem::path(path).parent_path());
#endif
        }

        std::string ResolveAssetsDir(const ApplicationSpec& spec) {
#if !defined(NDEBUG) && defined(MME_ASSETS_SOURCE_DIR)
            // Debug: repodaki asset'leri kullan; düzenlenen doku anında görünür,
            // kaydedilen sahneler build klasöründe kaybolmaz.
            if (spec.useSourceAssetsInDebug && std::filesystem::is_directory(MME_ASSETS_SOURCE_DIR))
                return MME_ASSETS_SOURCE_DIR;
#endif
            return spec.assetsDir;
        }

        bool IsMouseEvent(EventType t) {
            return t == EventType::MouseButtonPressed || t == EventType::MouseMoved || t == EventType::MouseScrolled;
        }
    }

    Application::Application(const ApplicationSpec& spec)
        : m_Spec(spec)
    {
        MME_ASSERT(s_Instance == nullptr, "Ayni anda yalnizca bir Application olabilir");
        MME_ASSERT(spec.fixedTimestep > 0.0f, "fixedTimestep pozitif olmali");
        s_Instance = this;

        SetWorkingDirectoryToExecutable();

        m_Window = std::make_unique<Window>(spec.window);
        m_Window->SetEventCallback([this](Event& e) { OnEvent(e); });

        const std::string assetsDir = ResolveAssetsDir(spec);
        MME_INFO("Assets: ", std::filesystem::absolute(assetsDir).string());
        Assets::Init(assetsDir);
        Renderer2D::Init();
        UI::Init();
        if (!spec.uiFont.empty()) {
            if (auto font = Font::LoadTTF(spec.uiFont, spec.uiFontBakeSize)) UI::SetFont(std::move(font));
        }
        if (spec.enableAudio) Audio::Init();
        ComponentRegistry::RegisterBuiltins();

        if (spec.enableDebugTools) {
            ImGuiBackend::Init(*m_Window, spec.imguiFont.empty() ? std::string() : Assets::GetPath(spec.imguiFont), spec.imguiFontSize);
            PushOverlay<DebugOverlay>(spec.showDebugOverlay);
            PushOverlay<DebugPanel>();
        }
    }

    Application::~Application() {
        // Katmanlar ters sırada sökülür; GPU kaynakları context hâlâ açıkken bırakılır.
        for (auto it = m_Layers.rbegin(); it != m_Layers.rend(); ++it)
            (*it)->OnDetach();
        m_Layers.clear();
        m_PendingAdd.clear();

        ImGuiBackend::Shutdown();
        Audio::Shutdown();
        UI::Shutdown();
        Renderer2D::Shutdown();
        Assets::Shutdown();
        m_Window.reset();
        s_Instance = nullptr;
    }

    Application& Application::Get() {
        MME_ASSERT(s_Instance, "Application henuz olusturulmadi");
        return *s_Instance;
    }

    // ---------------------------------------------------------------- katman yığını
    void Application::QueueLayer(std::unique_ptr<Layer> layer, bool overlay) {
        if (m_InLoop) m_PendingAdd.push_back({ std::move(layer), overlay });
        else AttachLayer(std::move(layer), overlay);
    }

    void Application::AttachLayer(std::unique_ptr<Layer> layer, bool overlay) {
        Layer* raw = layer.get();
        if (overlay) {
            m_Layers.push_back(std::move(layer));
        } else {
            m_Layers.insert(m_Layers.begin() + (std::ptrdiff_t)m_LayerInsertIndex, std::move(layer));
            ++m_LayerInsertIndex;
        }
        raw->OnAttach();
    }

    void Application::PopLayer(Layer& layer) {
        if (m_InLoop) { m_PendingRemove.push_back(&layer); return; }
        auto it = std::find_if(m_Layers.begin(), m_Layers.end(), [&](const auto& l) { return l.get() == &layer; });
        if (it == m_Layers.end()) return;
        (*it)->OnDetach();
        if ((size_t)(it - m_Layers.begin()) < m_LayerInsertIndex) --m_LayerInsertIndex;
        m_Layers.erase(it);
    }

    void Application::ApplyPendingLayers() {
        const bool wasInLoop = m_InLoop;
        m_InLoop = false; // aşağıdaki çağrılar doğrudan uygulansın
        for (Layer* l : m_PendingRemove) PopLayer(*l);
        m_PendingRemove.clear();
        // OnAttach yeni katman ekleyebilir: kuyruk boşalana kadar
        while (!m_PendingAdd.empty()) {
            auto pending = std::move(m_PendingAdd);
            m_PendingAdd.clear();
            for (auto& p : pending) AttachLayer(std::move(p.layer), p.overlay);
        }
        m_InLoop = wasInLoop;
    }

    // ---------------------------------------------------------------- olaylar
    void Application::OnEvent(Event& e) {
        // ImGui fareyi/klavyeyi kullanıyorsa oyun bu girdiyi görmez. Bırakma
        // olayları her zaman işlenir: aksi halde tuş basılı kalmış sayılırdı.
        const bool toImGui = (IsMouseEvent(e.type) && ImGuiBackend::WantsMouse()) ||
                             (e.type == EventType::KeyPressed && ImGuiBackend::WantsKeyboard());
        if (toImGui) return;

        Input::Internal::ProcessEvent(e);

        if (e.type == EventType::WindowClose)
            m_Running = false;

        for (auto it = m_Layers.rbegin(); it != m_Layers.rend() && !e.handled; ++it)
            (*it)->OnEvent(e);
    }

    // ---------------------------------------------------------------- ana döngü
    void Application::Run() {
        const float step = m_Spec.fixedTimestep;
        double lastTime = m_Window->GetTime();
        float accumulator = 0.0f;
        float hotReloadTimer = 0.0f;

        while (m_Running) {
            ApplyPendingLayers();
            m_InLoop = true;

            // Önce "bu frame" bayraklarını temizle, sonra yeni olayları topla
            Input::Internal::BeginFrame();
            if (m_Window->IsMinimized()) {
                m_Window->WaitEvents(0.1);
                lastTime = m_Window->GetTime(); // geri gelince dev bir dt oluşmasın
                m_InLoop = false;
                continue;
            }
            m_Window->PollEvents();

            const double now = m_Window->GetTime();
            // Pencere sürüklenirken / debugger'da beklerken simülasyon patlamasın
            m_FrameTime = std::min((float)(now - lastTime), 0.25f);
            lastTime = now;

            float simDt = m_Paused ? 0.0f : m_FrameTime * m_TimeScale;
            if (m_StepRequested) { simDt += step; m_StepRequested = false; }

            accumulator += simDt;
            while (accumulator >= step) {
                for (size_t i = 0; i < m_Layers.size(); ++i) m_Layers[i]->OnFixedUpdate(step);
                accumulator -= step;
            }
            for (size_t i = 0; i < m_Layers.size(); ++i) m_Layers[i]->OnUpdate(simDt);

            Audio::Update();
            hotReloadTimer += m_FrameTime;
            if (m_Spec.hotReload && hotReloadTimer >= 0.5f) {
                Assets::CheckForChanges();
                hotReloadTimer = 0.0f;
            }

            RenderFrame();
            m_InLoop = false;
        }
        ApplyPendingLayers();
    }

    void Application::RenderFrame() {
        const int w = m_Window->GetWidth();
        const int h = m_Window->GetHeight();

        Renderer2D::ResetStats();
        Renderer2D::SetViewport(w, h);
        Renderer2D::Clear(m_Spec.clearColor);

        for (size_t i = 0; i < m_Layers.size(); ++i) m_Layers[i]->OnRender();

        // UI geçişi: ekran pikseli, (0,0) sol-üst, +y aşağı
        Renderer2D::BeginScene(glm::ortho(0.0f, (float)w, (float)h, 0.0f));
        for (size_t i = 0; i < m_Layers.size(); ++i) m_Layers[i]->OnUIRender();
        Renderer2D::EndScene();

        if (m_Spec.enableDebugTools) {
            ImGuiBackend::BeginFrame();
            for (size_t i = 0; i < m_Layers.size(); ++i) m_Layers[i]->OnImGuiRender();
            ImGuiBackend::EndFrame();
        }

        m_Window->SwapBuffers();
    }
}
