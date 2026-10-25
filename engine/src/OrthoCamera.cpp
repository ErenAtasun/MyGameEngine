#include <MME/OrthoCamera.h>
#include <glm/gtc/matrix_transform.hpp>

namespace mme {

    OrthoCamera::OrthoCamera(float viewportWidth, float viewportHeight) {
        SetViewportSize(viewportWidth, viewportHeight);
        RecalcView();
    }

    void OrthoCamera::SetViewportSize(float width, float height) {
        // Simge durumunda 0 gelebilir; sıfıra bölmeyi önle
        m_ViewportW = width > 0.0f ? width : 1.0f;
        m_ViewportH = height > 0.0f ? height : 1.0f;
        RecalcProjection();
    }

    void OrthoCamera::SetZoom(float zoom) {
        m_Zoom = zoom > 0.0001f ? zoom : 0.0001f;
        RecalcProjection();
    }

    void OrthoCamera::RecalcProjection() {
        const float halfW = m_ViewportW * 0.5f * m_Zoom;
        const float halfH = m_ViewportH * 0.5f * m_Zoom;
        m_Proj = glm::ortho(-halfW, halfW, -halfH, halfH);
    }

    void OrthoCamera::RecalcView() {
        // Kameranın dünya dönüşümünün tersi: önce konuma taşı, sonra döndür
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(m_Position, 0.0f));
        transform = glm::rotate(transform, glm::radians(m_Rotation), glm::vec3(0, 0, 1));
        m_View = glm::inverse(transform);
    }

    glm::vec2 OrthoCamera::ScreenToWorld(const glm::vec2& screenPx) const {
        const glm::vec4 ndc{
            (screenPx.x / m_ViewportW) * 2.0f - 1.0f,
            1.0f - (screenPx.y / m_ViewportH) * 2.0f,
            0.0f, 1.0f
        };
        const glm::vec4 world = glm::inverse(GetViewProjection()) * ndc;
        return { world.x, world.y };
    }
}
