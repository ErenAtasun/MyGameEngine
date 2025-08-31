#pragma once
#include "OrthoCamera.h"
#include <GLFW/glfw3.h>

class OrthoCameraController {
public:
    OrthoCameraController(float width, float height);

    void OnResize(float width, float height);
    void OnScroll(double yoffset);               // mouse wheel
    void OnUpdate(float dt, GLFWwindow* win);    // WASD

    OrthoCamera& GetCamera() { return m_Camera; }

private:
    OrthoCamera m_Camera;
    float m_Width, m_Height;
    float m_MoveSpeed = 500.0f;   // px/sn
    float m_ZoomStep = 0.1f;     // her çevirmede
};
