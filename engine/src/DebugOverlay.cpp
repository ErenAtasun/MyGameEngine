#include "DebugOverlay.h"
#include <MME/Application.h>
#include <MME/UI.h>

#include <cstdio>

namespace mme {

    void DebugOverlay::OnUpdate(float /*dt*/) {
        // Update, çizimden önce çalışır: buradaki istatistikler önceki frame'in tamamıdır
        m_LastStats = Renderer2D::GetStats();
        // Ölçeklenmiş dt değil gerçek süre: oyun duraklatılınca FPS sıfır görünmesin
        const float real = Application::Get().GetFrameTime();
        m_SmoothedDt += (real - m_SmoothedDt) * 0.05f;
    }

    void DebugOverlay::OnEvent(Event& e) {
        if (e.type == EventType::KeyPressed && e.key == Key::F3 && !e.repeat) {
            m_Visible = !m_Visible;
            e.handled = true;
        }
    }

    void DebugOverlay::OnUIRender() {
        if (!m_Visible) return;

        char text[160];
        std::snprintf(text, sizeof(text),
            "FPS %.0f (%.2f ms)\nDraw calls %u\nQuads %u\nTexture binds %u",
            m_SmoothedDt > 0.0f ? 1.0f / m_SmoothedDt : 0.0f, m_SmoothedDt * 1000.0f,
            m_LastStats.drawCalls, m_LastStats.quadCount, m_LastStats.textureBinds);

        const float size = 14.0f;
        const glm::vec2 pos{ 12.0f, 12.0f };
        const glm::vec2 textSize = UI::MeasureText(text, size);
        Renderer2D::DrawScreenRect(pos - glm::vec2(6.0f), textSize + glm::vec2(12.0f), { 0.0f, 0.0f, 0.0f, 0.65f });
        UI::Text(text, pos, size, { 0.6f, 1.0f, 0.6f, 1.0f });
    }
}
