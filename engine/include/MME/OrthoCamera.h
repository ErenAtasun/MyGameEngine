#pragma once
#include <glm/glm.hpp>

namespace mme {

    // Merkez tabanlı ortografik kamera. 1 dünya birimi = zoom 1'de 1 piksel.
    // position, ekranın ortasına gelen dünya noktasıdır; zoom > 1 uzaklaştırır.
    class OrthoCamera {
    public:
        OrthoCamera(float viewportWidth, float viewportHeight);

        void SetViewportSize(float width, float height);
        void SetPosition(const glm::vec2& p) { m_Position = p; RecalcView(); }
        void SetRotation(float degrees) { m_Rotation = degrees; RecalcView(); }
        void SetZoom(float zoom);

        const glm::vec2& GetPosition() const { return m_Position; }
        float GetRotation() const { return m_Rotation; }
        float GetZoom() const { return m_Zoom; }
        glm::vec2 GetViewportSize() const { return { m_ViewportW, m_ViewportH }; }
        // Görünen dünya alanının boyutu (zoom dahil, rotasyon hariç)
        glm::vec2 GetVisibleSize() const { return { m_ViewportW * m_Zoom, m_ViewportH * m_Zoom }; }

        const glm::mat4& GetProjection() const { return m_Proj; }
        const glm::mat4& GetView() const { return m_View; }
        glm::mat4 GetViewProjection() const { return m_Proj * m_View; }

        // Ekran pikseli ((0,0) sol-üst) -> dünya koordinatı
        glm::vec2 ScreenToWorld(const glm::vec2& screenPx) const;

    private:
        void RecalcProjection();
        void RecalcView();

        glm::mat4 m_Proj{ 1.0f };
        glm::mat4 m_View{ 1.0f };
        glm::vec2 m_Position{ 0.0f };
        float m_Rotation = 0.0f; // derece
        float m_Zoom = 1.0f;
        float m_ViewportW = 1.0f;
        float m_ViewportH = 1.0f;
    };
}
