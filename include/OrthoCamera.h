#pragma once
#include <glm/glm.hpp>

class OrthoCamera {
public:
    OrthoCamera(float left, float right, float bottom, float top);

    void SetProjection(float left, float right, float bottom, float top);
    void SetPosition(const glm::vec2& p) { m_Position = p; RecalcView(); }
    void SetRotation(float r) { m_Rotation = r; RecalcView(); }
    void SetZoom(float z) { m_Zoom = z; RecalcView(); }

    const glm::vec2& GetPosition() const { return m_Position; }
    float GetRotation() const { return m_Rotation; }
    float GetZoom()     const { return m_Zoom; }

    const glm::mat4& GetProjection() const { return m_Proj; }
    const glm::mat4& GetView()       const { return m_View; }
    glm::mat4 GetViewProjection() const { return m_Proj * m_View; }

private:
    void RecalcView();

    glm::mat4 m_Proj{ 1.0f };
    glm::mat4 m_View{ 1.0f };
    glm::vec2 m_Position{ 0.0f };
    float m_Rotation = 0.0f; // derece
    float m_Zoom = 1.0f;
};
