#pragma once
#include <MME/Layer.h>
#include <MME/Renderer2D.h>

namespace mme {

    // F3 ile açılıp kapanan istatistik paneli (FPS, draw call, quad).
    // Application, diğer tüm katmanların üstüne ekler.
    class DebugOverlay : public Layer {
    public:
        explicit DebugOverlay(bool visible) : Layer("DebugOverlay"), m_Visible(visible) {}

        void OnUpdate(float dt) override;
        void OnUIRender() override;
        void OnEvent(Event& e) override;

    private:
        bool m_Visible;
        float m_SmoothedDt = 1.0f / 60.0f;
        Renderer2D::Stats m_LastStats;
    };
}
