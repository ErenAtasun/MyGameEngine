#pragma once
#include <MME/Event.h>
#include <string>
#include <utility>

namespace mme {

    // Oyunun ya da bir aracın (debug paneli vb.) motorla konuştuğu birim.
    // Application, katmanları eklendikleri sırayla günceller ve çizer;
    // olayları ise en üstteki katmandan başlayarak dağıtır.
    class Layer {
    public:
        explicit Layer(std::string name = "Layer") : m_Name(std::move(name)) {}
        virtual ~Layer() = default;

        virtual void OnAttach() {}
        virtual void OnDetach() {}

        // Sabit adımlı (varsayılan 60 Hz) — fizik ve oyun simülasyonu
        virtual void OnFixedUpdate(float /*step*/) {}
        // Frame başına bir kez — girdi, animasyon, kamera. dt zaman ölçeğiyle
        // çarpılmıştır (duraklatılınca 0); gerçek süre: Application::GetFrameTime()
        virtual void OnUpdate(float /*dt*/) {}
        // Dünya çizimi: kamera ile Renderer2D::BeginScene/EndScene'i katman yapar
        virtual void OnRender() {}
        // Ekran çizimi: motor (0,0) sol-üst, framebuffer pikseli projeksiyonunu hazırlar
        virtual void OnUIRender() {}
        // Dear ImGui pencereleri (araçlar, debug); ImGui:: çağrıları burada yapılır
        virtual void OnImGuiRender() {}

        virtual void OnEvent(Event& /*e*/) {}

        const std::string& GetName() const { return m_Name; }

    private:
        std::string m_Name;
    };
}
