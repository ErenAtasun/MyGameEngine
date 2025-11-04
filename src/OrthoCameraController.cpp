#include "OrthoCameraController.h"

OrthoCameraController::OrthoCameraController(float w, float h)
    : m_Camera(0.0f, w, 0.0f, h), m_Width(w), m_Height(h) {}

void OrthoCameraController::OnResize(float w, float h) {
    m_Width = w; m_Height = h;
    m_Camera.SetProjection(0.0f, w, 0.0f, h);
}

void OrthoCameraController::OnScroll(double y) {
    float z = m_Camera.GetZoom();
    z -= (float)y * m_ZoomStep;
    if (z < 0.1f) z = 0.1f;
    if (z > 10.0f) z = 10.0f;
    m_Camera.SetZoom(z);
}

void OrthoCameraController::OnUpdate(float dt, GLFWwindow* win) {
    auto p = m_Camera.GetPosition();
    // Map controls moved to numpad: 8 (up), 2 (down), 4 (left), 6 (right)
    if (glfwGetKey(win, GLFW_KEY_KP_8) == GLFW_PRESS) p.y += m_MoveSpeed * dt;
    if (glfwGetKey(win, GLFW_KEY_KP_2) == GLFW_PRESS) p.y -= m_MoveSpeed * dt;
    if (glfwGetKey(win, GLFW_KEY_KP_4) == GLFW_PRESS) p.x -= m_MoveSpeed * dt;
    if (glfwGetKey(win, GLFW_KEY_KP_6) == GLFW_PRESS) p.x += m_MoveSpeed * dt;
    m_Camera.SetPosition(p);
}
