#include <MME/OrthoCameraController.h>
#include <MME/Input.h>

#include <algorithm>

namespace mme {

    OrthoCameraController::OrthoCameraController(float w, float h)
        : m_Camera(w, h) {}

    void OrthoCameraController::OnUpdate(float dt) {
        glm::vec2 dir{ 0.0f };
        if (Input::KeyDown(keyUp))    dir.y += 1.0f;
        if (Input::KeyDown(keyDown))  dir.y -= 1.0f;
        if (Input::KeyDown(keyLeft))  dir.x -= 1.0f;
        if (Input::KeyDown(keyRight)) dir.x += 1.0f;
        if (dir.x != 0.0f || dir.y != 0.0f)
            m_Camera.SetPosition(m_Camera.GetPosition() + glm::normalize(dir) * moveSpeed * m_Camera.GetZoom() * dt);
    }

    void OrthoCameraController::OnEvent(Event& e) {
        if (e.type == EventType::MouseScrolled) {
            m_Camera.SetZoom(std::clamp(m_Camera.GetZoom() - e.y * zoomStep, minZoom, maxZoom));
        } else if (e.type == EventType::WindowResize) {
            m_Camera.SetViewportSize((float)e.width, (float)e.height);
        }
    }
}
