#pragma once
#include <MME/Layer.h>
#include <MME/Window.h>

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mme {

    struct ApplicationSpec {
        WindowSpec window;
        std::string assetsDir = "assets";        // çalıştırılabilirin yanındaki klasör
        bool useSourceAssetsInDebug = true;      // Debug: asset'ler doğrudan repodan (hot-reload için)
        float fixedTimestep = 1.0f / 60.0f;      // OnFixedUpdate adımı (saniye)
        glm::vec4 clearColor{ 0.10f, 0.11f, 0.15f, 1.0f };

        bool enableImGui = true;                 // Dear ImGui (OnImGuiRender); debug araçları bunu gerektirir
        bool imguiDocking = false;               // ImGui pencerelerini yerleştirme (editör)
        bool enableDebugTools = true;            // F1 debug paneli + F3 istatistik overlay
        bool showDebugOverlay = false;           // F3 başlangıçta açık mı
        bool enableAudio = true;
        bool hotReload = true;                   // değişen dokuları otomatik yeniden yükle

        std::string uiFont;                      // boş: yerleşik 5x7 font; ör. "fonts/FiraSans-Regular.ttf"
        float uiFontBakeSize = 48.0f;
        std::string imguiFont = "fonts/FiraSans-Regular.ttf"; // yoksa ImGui'nin kendi fontu
        float imguiFontSize = 17.0f;
    };

    // Motorun sahibi: pencereyi, alt sistemleri ve ana döngüyü yönetir.
    //
    //   mme::Application app(spec);
    //   app.PushLayer<MyGameLayer>();
    //   app.Run();
    class Application {
    public:
        explicit Application(const ApplicationSpec& spec = {});
        ~Application();
        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;

        // Oyun katmanları overlay'lerin altına eklenir. Döngü sırasında çağrılırsa
        // ekleme bir sonraki frame'in başında yapılır (OnAttach o zaman çağrılır).
        template<typename T, typename... Args>
        T& PushLayer(Args&&... args) {
            auto layer = std::make_unique<T>(std::forward<Args>(args)...);
            T& ref = *layer;
            QueueLayer(std::move(layer), false);
            return ref;
        }

        // Overlay'ler her zaman en üsttedir: en son çizilir, olayları ilk görür.
        template<typename T, typename... Args>
        T& PushOverlay(Args&&... args) {
            auto layer = std::make_unique<T>(std::forward<Args>(args)...);
            T& ref = *layer;
            QueueLayer(std::move(layer), true);
            return ref;
        }

        // Katmanı söker (OnDetach) ve yok eder; döngü sırasında frame sonuna ertelenir.
        void PopLayer(Layer& layer);

        void Run();
        void Close() { m_Running = false; }

        // Zaman: oyun katmanlarına giden dt ölçeklenir; duraklatınca 0 olur.
        void SetTimeScale(float scale) { m_TimeScale = scale < 0.0f ? 0.0f : scale; }
        float GetTimeScale() const { return m_TimeScale; }
        void SetPaused(bool paused) { m_Paused = paused; }
        bool IsPaused() const { return m_Paused; }
        void StepOneFrame() { m_StepRequested = true; } // duraklatılmışken tek sabit adım ilerlet
        float GetFrameTime() const { return m_FrameTime; } // gerçek (ölçeksiz) son frame süresi

        // true (varsayılan): ImGui fareyi/klavyeyi kullanırken (bir pencerenin üstünde,
        // metin kutusunda) oyun bu girdiyi görmez. Editör, oyun görünümü odaktayken
        // kapatır: oyun, viewport penceresinin üstünde olsa da girdiyi alır.
        void SetImGuiInputCapture(bool enabled) { m_ImGuiInputCapture = enabled; }

        Window& GetWindow() { return *m_Window; }
        const ApplicationSpec& GetSpec() const { return m_Spec; }
        const std::vector<std::unique_ptr<Layer>>& GetLayers() const { return m_Layers; }

        static Application& Get();

    private:
        void QueueLayer(std::unique_ptr<Layer> layer, bool overlay);
        void AttachLayer(std::unique_ptr<Layer> layer, bool overlay);
        void ApplyPendingLayers();
        void OnEvent(Event& e);
        void RenderFrame();

        ApplicationSpec m_Spec;
        std::unique_ptr<Window> m_Window;
        std::vector<std::unique_ptr<Layer>> m_Layers;
        size_t m_LayerInsertIndex = 0;   // [0, index) oyun katmanları, [index, end) overlay'ler

        struct PendingLayer { std::unique_ptr<Layer> layer; bool overlay; };
        std::vector<PendingLayer> m_PendingAdd;
        std::vector<Layer*> m_PendingRemove;
        bool m_InLoop = false;

        bool m_Running = true;
        bool m_Paused = false;
        bool m_StepRequested = false;
        float m_TimeScale = 1.0f;
        float m_FrameTime = 0.0f;
        bool m_ImGuiEnabled = false;
        bool m_ImGuiInputCapture = true;

        static Application* s_Instance;
    };
}
